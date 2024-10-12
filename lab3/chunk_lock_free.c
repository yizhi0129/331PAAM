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

_Atomic(struct chunk*) free_list = NULL;

struct chunk* pop() 
{
    struct chunk* head;

    do {
        head = atomic_load(&free_list);
        if (!head) 
        {
            return NULL; 
        }
    } while (!atomic_compare_exchange_weak(&free_list, &head, head->next));

    return head;
}

void push(struct chunk* head, struct chunk* tail) 
{
    struct chunk* old_head;
    do {
        old_head = atomic_load(&free_list);
        tail->next = old_head;
    } while (!atomic_compare_exchange_weak(&free_list, &old_head, head));
}

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

struct chunk* alloc_chunk(size_t size) 
{
    struct chunk* c;
    while ((c = pop()))
    {
        if (c->size >= size) 
        {
            if (c->size > size + sizeof(struct chunk)) 
            {
                struct chunk* new_chunk = (struct chunk*)((uintptr_t)c + sizeof(struct chunk) + size);
                new_chunk->size = c->size - size - sizeof(struct chunk);
                c->size = size;

                free_chunk(new_chunk);
            }
            return c; 
        }
        free_chunk(c);
    }

    struct chunk* large_chunk = create_large_chunk();
    if (!large_chunk) 
    {
        return NULL;
    }

    free_chunk(large_chunk);
    return alloc_chunk(size);
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
