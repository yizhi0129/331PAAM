#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 256
#define NUM_CONSUMERS 3  // Number of consumers

// Node structure for the linked-list queue
typedef struct Node {
    char *line;
    struct Node *next;
    int consumers_left;  // Number of consumers that still need to process this node
} Node;

// Queue structure with a pointer to the first and last node
typedef struct {
    Node *head;
    Node *tail;
} Queue;

// Shared queue and synchronization primitives
Queue queue = {NULL, NULL};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Enqueue function: adds a new line to the queue
void enqueue(Queue *q, const char *line) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->line = strdup(line);  // Copy the input line
    new_node->next = NULL;
    new_node->consumers_left = NUM_CONSUMERS;  // Set the number of consumers who need to process this node

    pthread_mutex_lock(&mutex);

    // If the queue is empty, head and tail both point to the new node
    if (q->tail == NULL) {
        q->head = q->tail = new_node;
    } else {
        q->tail->next = new_node;
        q->tail = new_node;
    }

    // Signal all consumers that a new item is available
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

// Function to be called by each consumer thread
void* consumer_function(void* arg) {
    int consumer_id = *((int*)arg);
    Node *current_node = NULL;

    while (1) {
        pthread_mutex_lock(&mutex);

        // If current_node is NULL, start from the head of the queue
        if (current_node == NULL) {
            current_node = queue.head;
        } else {
            current_node = current_node->next;  // Move to the next node
        }

        // Wait until there is a node to process
        while (current_node == NULL) {
            pthread_cond_wait(&cond, &mutex);
            current_node = queue.head;  // Recheck from the head when signaled
        }

        // Print the line processed by this consumer
        printf("[Consumer %d] Consumed: %s", consumer_id, current_node->line);

        // Decrement the consumers_left counter for this node
        current_node->consumers_left--;

        // If all consumers have processed this node, remove and free it
        if (current_node->consumers_left == 0) {
            if (queue.head == current_node) {
                queue.head = current_node->next;  // Move the head to the next node
                if (queue.head == NULL) {
                    queue.tail = NULL;  // If the queue is now empty, reset the tail
                }
            }
            free(current_node->line);
            free(current_node);
        }

        pthread_mutex_unlock(&mutex);

        // Sleep for a short time to simulate work
        usleep(100000);  // 100ms
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t consumers[NUM_CONSUMERS];
    int consumer_ids[NUM_CONSUMERS];

    // Create consumer threads
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumer_ids[i] = i + 1;  // Assign an ID to each consumer
        if (pthread_create(&consumers[i], NULL, consumer_function, &consumer_ids[i]) != 0) {
            perror("Failed to create consumer thread");
            return EXIT_FAILURE;
        }
    }

    // Producer: Read lines from standard input and enqueue them
    char input[MAX_LINE_LENGTH];
    while (fgets(input, MAX_LINE_LENGTH, stdin) != NULL) {
        enqueue(&queue, input);
    }

    // Join consumer threads
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    return 0;
}
