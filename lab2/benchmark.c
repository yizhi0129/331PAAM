#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

pthread_mutex_t lock;

// Global variable to track total time spent in the critical section by all threads
long long total_lock_time_ns = 0;  // in nanoseconds

// Mutex to protect updates to the total_lock_time_ns variable
pthread_mutex_t time_lock;

typedef struct {
    int tid;     // Thread id
    int iter;    // Number of iterations
    int csd;     // Critical section delay in nanoseconds
    int cd;    // Compute delay in nanoseconds
} thread_data_t;

// Function to wait for a given time in nanoseconds
void wait_ns(int ns) {
    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);  // Record the start time

    long int elapsed_ns = 0;
    while (elapsed_ns < ns) {  // Loop until the desired time has passed
        clock_gettime(CLOCK_MONOTONIC, &current);
        elapsed_ns = (current.tv_sec - start.tv_sec) * 1e9 + (current.tv_nsec - start.tv_nsec);
    }
}

// Benchmark function executed by each thread
void* benchmark(void* arg) {
    thread_data_t *data = (thread_data_t*) arg;
    long long lock_time_ns = 0;

    for (int i = 0; i < data->iter; ++i) {
        struct timespec lock_start, lock_end;

        // Record the time before acquiring the lock
        clock_gettime(CLOCK_MONOTONIC, &lock_start);

        pthread_mutex_lock(&lock);

        // Record the time after releasing the lock
        clock_gettime(CLOCK_MONOTONIC, &lock_end);

        // Calculate time spent in the critical section (locking time)
        long long time_in_lock = (lock_end.tv_sec - lock_start.tv_sec) * 1e9 +
                                 (lock_end.tv_nsec - lock_start.tv_nsec);

        // Add to the per-thread lock time
        lock_time_ns += time_in_lock;

        // Simulate the critical section delay
        wait_ns(data->csd);

        pthread_mutex_unlock(&lock);

        // Simulate the computation delay (outside the critical section)
        wait_ns(data->cd);
    }

    // Protect total_lock_time_ns with a mutex and update the global time
    pthread_mutex_lock(&time_lock);
    total_lock_time_ns += lock_time_ns;
    pthread_mutex_unlock(&time_lock);

    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <number of threads> <number of iterations> <critical section delay in ns> <compute delay in ns> <lock algorithm>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse arguments
    int n = atoi(argv[1]);      // Number of threads
    int it = atoi(argv[2]);     // Number of iterations per thread
    int csd = atoi(argv[3]);    // Critical section delay in nanoseconds
    int cd = atoi(argv[4]);   // Compute section delay in nanoseconds
    char *lock_algo = argv[5];  // Lock algorithm (not used here but can be extended)

    pthread_t threads[n];
    thread_data_t thread_data[n];

    // Initialize the mutex lock
    pthread_mutex_init(&lock, NULL);

    // Initialize the mutex for time calculation
    pthread_mutex_init(&time_lock, NULL);

    // Create threads
    for (int i = 0; i < n; ++i) {
        thread_data[i].tid = i;
        thread_data[i].iter = it;
        thread_data[i].csd = csd;
        thread_data[i].cd = cd;

        if (pthread_create(&threads[i], NULL, benchmark, (void*) &thread_data[i])) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return EXIT_FAILURE;
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < n; ++i) {
        if (pthread_join(threads[i], NULL)) {
            fprintf(stderr, "Error joining thread %d\n", i);
            return EXIT_FAILURE;
        }
    }

    // Report the total and average time taken to acquire and release the lock
    long long average_lock_time_ns = total_lock_time_ns / (n * it);

    printf("Total time spent in acquiring and releasing the lock: %lld ns\n", total_lock_time_ns);
    printf("Average time spent in acquiring and releasing the lock per operation: %lld ns\n", average_lock_time_ns);

    // Clean up
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&time_lock);

    return EXIT_SUCCESS;
}
