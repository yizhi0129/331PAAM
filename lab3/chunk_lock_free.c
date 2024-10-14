#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <time.h>

#define LARGE_CHUNK_SIZE (64 * 1024 * 1024)  

struct chunk {
    size_t size;  
    union {
        struct chunk* next;  
        char content[0];    
    };
};

// Atomic pointer to the free chunk list head
_Atomic(struct chunk*) free_list = NULL;

// Function to pop a chunk from the free list (lock-free, using CAS)
struct chunk* pop() 
{
    struct chunk* head;

    do {
        head = atomic_load(&free_list);  // Load the head of the free list atomically
        if (!head) 
        {
            return NULL;  // If the free list is empty, return NULL
        }
    } while (!atomic_compare_exchange_weak(&free_list, &head, head->next));  
    // CAS operation: Try to swap the head with its next chunk. If another thread modifies it, retry.

    return head;  // Return the popped chunk
}

// Function to push a chunk (or a list of chunks) onto the free list (lock-free, using CAS)
void push(struct chunk* head, struct chunk* tail) 
{
    struct chunk* old_head;
    do {
        old_head = atomic_load(&free_list);  // Load the current head of the free list atomically
        tail->next = old_head;  // Point the tail of the list to the current head
    } while (!atomic_compare_exchange_weak(&free_list, &old_head, head));
    // CAS operation: Atomically update the head of the free list to the new head (the chunk or list being pushed)
}

// Function to free (release) a chunk and push it onto the free list
void free_chunk(struct chunk* c) 
{
    push(c, c);  
}

struct chunk* create_large_chunk() 
{
    struct chunk* large_chunk = (struct chunk*)malloc(LARGE_CHUNK_SIZE);  
    if (!large_chunk) 
    {
        return NULL;  
    }

    large_chunk->size = LARGE_CHUNK_SIZE - sizeof(struct chunk);  
    return large_chunk;
}

// Function to allocate a chunk of the requested size from the free list
struct chunk* alloc_chunk(size_t size) 
{
    struct chunk* c;
    // Try to pop a chunk from the free list that is large enough
    while ((c = pop()))  
    {
        if (c->size >= size)  // If the chunk is large enough
        {
            // If the chunk is larger than needed, split it and return the required size
            if (c->size > size + sizeof(struct chunk)) 
            {
                struct chunk* new_chunk = (struct chunk*)((uintptr_t)c + sizeof(struct chunk) + size);
                new_chunk->size = c->size - size - sizeof(struct chunk);  
                c->size = size;  // Shrink the current chunk to the requested size

                free_chunk(new_chunk);  // Return the remaining part to the free list
            }
            return c;  // Return the allocated chunk
        }
        free_chunk(c);  // If the chunk is too small, return it to the free list
    }

    // If no suitable chunk is found, create a large chunk
    struct chunk* large_chunk = create_large_chunk();
    if (!large_chunk) 
    {
        return NULL;  // Return NULL if allocation failed
    }

    free_chunk(large_chunk);  // Add the newly created large chunk to the free list
    return alloc_chunk(size);  // Retry the allocation after adding the large chunk
}

double timer(struct timespec start, struct timespec end) 
{
    return (end.tv_sec - start.tv_sec) * 1e3 + (end.tv_nsec - start.tv_nsec) / 1e6;
}

int main(int argc, char** argv)
{
    size_t size = 1024;  
    if (argc == 2) 
    {
        size = atoi(argv[1]);  
        if (size > LARGE_CHUNK_SIZE)  // Ensure the requested size is not too large
        {
            printf("Requested size is too large\n");
            return 1;
        }
    }

    // Record the start time before calling alloc_chunk
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);  
    struct chunk* c = alloc_chunk(size);    // Allocate the chunk
    clock_gettime(CLOCK_MONOTONIC, &end_time);  // Record the end time

    // Print the allocation time in milliseconds
    printf("Allocation time: %.6f ms\n", timer(start_time, end_time));
    
    if (c) 
    {
        printf("Allocated a chunk of size %zu\n", c->size);  // Print the size of the allocated chunk
    }
    else 
    {
        printf("Failed to allocate a chunk of size %zu\n", size);  // Print an error message if allocation failed
    }

    return 0; 
}
