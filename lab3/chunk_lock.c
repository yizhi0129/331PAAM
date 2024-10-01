#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#define LARGE_CHUNK_SIZE (64 * 1024 * 1024)

struct chunk {
    size_t size;
    union {
        struct chunk* next; /* next node when the chunk is free */
        char content[0];    /* the content of the chunk when allocated */
    };
};

struct chunk* free_list = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void free_chunk(struct chunk* c)
{
    pthread_mutex_lock(&mutex);
    c->next = free_list;
    free_list = c;
    pthread_mutex_unlock(&mutex);   
};


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
    pthread_mutex_lock(&mutex);
    struct chunk** curr = &free_list;
    struct chunk* prev = NULL;

    while (*curr && (*curr)->size < size) 
    {
        prev = *curr;
        curr = &(*curr)->next;
    }

    if (!*curr) 
    {
        struct chunk* large_chunk = create_large_chunk();
        if (!large_chunk) 
        {
            pthread_mutex_unlock(&mutex);
            return NULL; 
        }
        free_chunk(large_chunk);

        pthread_mutex_unlock(&mutex);
        return alloc_chunk(size);
    }

    struct chunk* selected_chunk = *curr;

    if (prev) 
    {
        prev->next = selected_chunk->next;
    } 
    else 
    {
        free_list = selected_chunk->next;
    }

    if (selected_chunk->size > size + sizeof(struct chunk)) 
    {
        struct chunk* new_chunk = (struct chunk*)((uintptr_t)selected_chunk + sizeof(struct chunk) + size);
        new_chunk->size = selected_chunk->size - size - sizeof(struct chunk);

        selected_chunk->size = size;

        free_chunk(new_chunk);
    }
    pthread_mutex_unlock(&mutex);
    return selected_chunk;
}

int main(int argc, char** argv)
{

    return 0;
}