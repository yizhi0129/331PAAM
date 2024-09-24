#!/bin/bash

gcc -o s_1 solution_1.c -pthread
gcc -o s_2 solution_2.c -pthread
gcc -o s_3a solution_3a.c -pthread
gcc -o s_3bc solution_3bc.c -pthread

gcc -o t_a thread_app.c -pthread
gcc -o p_c producer_consumer.c -pthread
gcc -o m_q multicast_queue.c -pthread
gcc -o d_q desync_queue.c -pthread