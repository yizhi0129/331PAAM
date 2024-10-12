#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

#define LARGE_CHUNK_SIZE (64 * 1024 * 1024)

struct chunk* free_list = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


struct chunk {
    size_t size;
    union {
        struct chunk* next; /* next node when the chunk is free */
        char content[0];    /* the content of the chunk when allocated */
    };
};

void free_chunk(struct chunk* c)
{
    pthread_mutex_lock(&mutex);
    c->next = free_list;  // add the chunk to the free list
    free_list = c;        // update the head of the free list
    pthread_mutex_unlock(&mutex);   
};

struct chunk* create_large_chunk() 
{
    struct chunk* large_chunk = (struct chunk*)malloc(LARGE_CHUNK_SIZE);
    if (!large_chunk) 
    {
        return NULL; 
    } 
    large_chunk->size = LARGE_CHUNK_SIZE - sizeof(struct chunk);    // usable size
    return large_chunk;
}

struct chunk* alloc_chunk(size_t size) 
{   
    struct chunk* curr = free_list;
    while (curr) 
    {
        if (curr->size >= size + sizeof(struct chunk))  // if a large enough chunk is found
        {
            free_list = curr->next;  // remove the chunk from the free list
            if (curr->size > size + sizeof(struct chunk))
            {
                struct chunk* rest = (struct chunk*)((uintptr_t)curr + size + sizeof(struct chunk));
                rest->size = curr->size - size - sizeof(struct chunk); 
                curr->size = size;
                free_chunk(rest); // add the remaining chunk to the free list
            }
            return curr;
        }
        else  
        {
            curr = curr->next; // search the next chunk
        }
    }
    
    // if no suitable chunk is found, create a large chunk
    struct chunk* large_chunk = create_large_chunk(); 
    if (!large_chunk) 
    {
        return NULL; 
    }
    free_chunk(large_chunk);   // add the large chunk to the free list
    return alloc_chunk(size);  // repeat searching
}

double timer(struct timespec start, struct timespec end) 
{
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e6;
}

int main(int argc, char** argv)
{
    size_t size = 1024;
    if (argc == 2) 
    {
        size = atoi(argv[1]);
        if (size > LARGE_CHUNK_SIZE) 
        {
            printf("Requested size is too large\n");
            return 1;
        }
    }
    // measure the time of allocation
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    struct chunk* c = alloc_chunk(size);    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    printf("Allocation time: %.6f ms\n", timer(start_time, end_time));
    
    if (c) 
    {
        printf("Allocated a chunk of size %zu\n", c->size);
    }
    else 
    {
        printf("Failed to allocate a chunk of size %zu\n", size);
    }
    return 0;
}