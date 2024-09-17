#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Create a counter for the number of threads
int counter = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void count() {
    pthread_mutex_lock(&m);
    counter++;
    pthread_mutex_unlock(&m);
}

// Structure to pass data to each thread
typedef struct {
    int index;
} thread_data_t;

// Thread function that prints the required message
void* thread_function(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    count();
    printf("[%lu] Hello, World %d!!!\n", (unsigned long)pthread_self(), data->index);
    free(data);  // Free the dynamically allocated memory
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Get the number of threads to create from the command-line argument
    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Error: Number of threads must be a positive integer.\n");
        return EXIT_FAILURE;
    }

    // Array to hold the thread IDs
    pthread_t threads[num_threads];

    // Create N threads
    for (int i = 0; i < num_threads; i++) {
        // Allocate memory for the thread-specific data
        thread_data_t* data = malloc(sizeof(thread_data_t));
        if (data == NULL) {
            fprintf(stderr, "Error: Could not allocate memory.\n");
            return EXIT_FAILURE;
        }

        data->index = i;

        // Create the thread
        int result = pthread_create(&threads[i], NULL, thread_function, (void*)data);
        if (result != 0) {
            fprintf(stderr, "Error: Could not create thread %d.\n", i);
            return EXIT_FAILURE;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // After all threads have finished, print the final message
    if (counter == num_threads) {
        printf("Total thread count = %d\nMain quitting.\n", counter);
    } else {
        printf("Error: Counter does not match the number of threads.\n");
    }

    // Destroy the mutex
    pthread_mutex_destroy(&m);

    return EXIT_SUCCESS;
}
