#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include </usr/include/hwloc.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define CACHE_LINE_SIZE 64
#define NOISY_BUFFER_SIZE (32 * 1024 * 1024)  // 32 MB buffer size for noisy threads

// Define the struct for cache lines as per provided requirements
struct cache_line {
    union {
        char _content[CACHE_LINE_SIZE]; // 64 bytes in each cache line
        int value;
    };
};

// Enum for noisy configurations
typedef enum {
    NOISY_NONE,
    NOISY_SPREAD,
    NOISY_OVERLOAD
} noisy_config_t;

// Global topology variable for hwloc
hwloc_topology_t topology;

// Find the NUMA node for a processing unit
hwloc_obj_t pu_to_node(hwloc_obj_t cur) 
{
    assert(cur->type == HWLOC_OBJ_PU);
    do {
        cur = cur->parent;
    } while(cur && cur->type != HWLOC_OBJ_NUMANODE);
    return cur;
}

// Function for noisy thread to continuously access memory
void *noisy_thread(void *arg) 
{
    struct cache_line *buffer = (struct cache_line *)arg;
    int num_lines = NOISY_BUFFER_SIZE / sizeof(struct cache_line);
    volatile int r = 0; // Avoid compiler optimization

    while (1) {
        for (int i = 0; i < num_lines; i++) {
            r += buffer[i].value;
        }
    }
    return NULL;
}

// Test thread to access the buffer repeatedly and measure access time
void *test_thread(void *arg) 
{
    struct cache_line *buffer = (struct cache_line *)arg;
    int num_accesses = NOISY_BUFFER_SIZE / CACHE_LINE_SIZE;  // Determine number of accesses
    volatile int r = 0;

    for (int i = 0; i < num_accesses; i++) {
        r += buffer[i % num_accesses].value;
    }
    return NULL;
}

int num_nodes;  // Global variable to store the number of NUMA nodes

// Initialize the hwloc topology
void init_topology() 
{
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);
    num_nodes = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
    printf("System has %d NUMA nodes.\n", num_nodes);
}

// Allocate memory on a specific NUMA node
void *allocate_numa_memory(size_t size, int node) 
{
    if (node >= num_nodes) 
    {
        fprintf(stderr, "Error: NUMA node %d does not exist. System only has %d NUMA nodes.\n", node, num_nodes);
        return NULL;
    }
    hwloc_obj_t obj = hwloc_get_obj_by_depth(topology, hwloc_get_type_depth(topology, HWLOC_OBJ_NODE), node);
    if (!obj) 
    {
        fprintf(stderr, "Error: Failed to retrieve NUMA node %d.\n", node);
        return NULL;
    }
    return hwloc_alloc_membind(topology, size, obj->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_STRICT);
}

int main(int argc, char **argv) 
{
    if (argc < 6) {
        printf("Usage: %s <test_pu> <test_node> <test_buffer_kb> <test_workload_kb> <noisy_config> [noisy_node]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int test_pu = atoi(argv[1]);
    int test_node = atoi(argv[2]);
    size_t test_buffer_kb = atoi(argv[3]) * 1024;
    size_t test_workload_kb = atoi(argv[4]) * 1024;
    noisy_config_t noisy_config = (strcmp(argv[5], "none") == 0) ? NOISY_NONE : 
                                  (strcmp(argv[5], "spread") == 0) ? NOISY_SPREAD : NOISY_OVERLOAD;
    int noisy_node = (argc == 7) ? atoi(argv[6]) : -1;

    init_topology();

    // Allocate and bind test buffer to NUMA node
    struct cache_line *test_buffer = (struct cache_line *)allocate_numa_memory(test_buffer_kb, test_node);
    if (!test_buffer) {
        perror("Failed to allocate test buffer");
        return EXIT_FAILURE;
    }

    // Create and bind test thread
    pthread_t test_tid;
    pthread_attr_t test_attr;
    pthread_attr_init(&test_attr);
    hwloc_obj_t pu = hwloc_get_obj_by_depth(topology, hwloc_get_type_depth(topology, HWLOC_OBJ_PU), test_pu);
    hwloc_set_cpubind(topology, pu->cpuset, HWLOC_CPUBIND_THREAD);
    pthread_create(&test_tid, &test_attr, test_thread, test_buffer);

    // Start noisy threads based on configuration
    int num_pus = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    pthread_t noisy_threads[num_pus];
    if (noisy_config != NOISY_NONE) 
    {
        for (int i = 0; i < num_pus; i++) 
        {
            if (i == test_pu) continue;

            int node = (noisy_config == NOISY_SPREAD) ? (i + 1) % num_pus : noisy_node;
            struct cache_line *noisy_buffer = (struct cache_line *)allocate_numa_memory(NOISY_BUFFER_SIZE, node);
            if (!noisy_buffer) 
            {
                perror("Failed to allocate noisy buffer");
                continue;
            }

            pthread_create(&noisy_threads[i], NULL, noisy_thread, noisy_buffer);
        }
    }

    // Measure time for test thread access
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_join(test_tid, NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate and print elapsed time
    double elapsed_time = (end.tv_sec - start.tv_sec) * 1e3 + (end.tv_nsec - start.tv_nsec) / 1e6;
    printf("Test memory access time: %.6f ms\n", elapsed_time);

    // Clean up
    hwloc_topology_destroy(topology);
    free(test_buffer);
    return EXIT_SUCCESS;
}
