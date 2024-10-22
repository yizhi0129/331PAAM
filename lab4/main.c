#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "mem.h"

int main(int argc, char** argv)
{
    if (argc != 2) 
    {
        fprintf(stderr, "Usage: %s <N threads>\n", argv[0]);
        return 1;
    }
    int N_threads = atoi(argv[1]);
    if (N_threads <= 0) 
    {
        fprintf(stderr, "N threads must be a positive integer\n");
        return 1;
    }

    return 0;
}