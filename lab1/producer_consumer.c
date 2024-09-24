#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 256

// Node structure for the linked-list queue
typedef struct Node {
    char *line;
    struct Node *next;
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
void enqueue(Queue *q, const char *line) 
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->line = strdup(line); // Copy the input line //动态分配内存来存储新复制的字符串，并将其返回 
                                                            //自动为目标字符串分配所需的内存，不必手动计算长度和分配空间
    new_node->next = NULL;

    pthread_mutex_lock(&mutex);

    // If the queue is empty, head and tail both point to the new node
    if (q->tail == NULL) 
    {
        q->head = q->tail = new_node;
    } 
    else 
    {
        q->tail->next = new_node;
        q->tail = new_node;
    }

    // Signal the consumer that a new item is available
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

// Dequeue function: removes and returns the oldest line in the queue
char* dequeue(Queue *q) 
{
    pthread_mutex_lock(&mutex);

    // Wait if the queue is empty
    while (q->head == NULL) 
    {
        pthread_cond_wait(&cond, &mutex);
    }

    Node *temp = q->head;
    char *line = strdup(temp->line);  // Copy the line to return
    q->head = q->head->next;          // Move the head pointer

    // If the queue is now empty, reset the tail pointer
    if (q->head == NULL) 
    {
        q->tail = NULL;
    }

    free(temp->line);  // Free the line memory
    free(temp);        // Free the node
    pthread_mutex_unlock(&mutex);

    return line;
}

// Consumer thread function
void* consumer_function(void* arg) 
{
    while (1) 
    {
        // Dequeue and print the line
        char *line = dequeue(&queue);
        printf("Consumed: %s", line);
        free(line);  // Free the dequeued line
    }

    pthread_exit(NULL);
}

int main(int argc, char** argv) 
{
    pthread_t consumer_thread;

    // Start the consumer thread
    if (pthread_create(&consumer_thread, NULL, consumer_function, NULL) != 0) {
        perror("Failed to create consumer thread");
        return EXIT_FAILURE;
    }

    // Producer: Read lines from standard input and enqueue them
    char input[MAX_LINE_LENGTH];
    while (fgets(input, MAX_LINE_LENGTH, stdin) != NULL) {
        enqueue(&queue, input);
    }

    // Join the consumer thread (in a real program, you'd have a condition to stop)
    pthread_join(consumer_thread, NULL);

    return 0;
}
