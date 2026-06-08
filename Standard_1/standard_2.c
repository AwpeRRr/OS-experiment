/*
 * Experiment 2: thread shared data.
 *
 * Compile on Ubuntu:
 *     gcc standard_2.c -o standard_2 -pthread
 *
 * Run:
 *     ./standard_2
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int shared_data = 0;
pthread_mutex_t lock;

void *thread_function(void *arg)
{
    int id = *(int *)arg;
    int private_data = id * 100;

    /* Each thread has its own local variable. */
    printf("Thread %d starts, private_data = %d\n", id, private_data);

    /* The global variable is shared by all threads in this process. */
    pthread_mutex_lock(&lock);
    shared_data = shared_data + 10;
    printf("Thread %d changes shared_data to %d\n", id, shared_data);
    pthread_mutex_unlock(&lock);

    return NULL;
}

int main(void)
{
    pthread_t thread1;
    pthread_t thread2;
    int id1 = 1;
    int id2 = 2;

    pthread_mutex_init(&lock, NULL);

    printf("Main thread starts, shared_data = %d\n", shared_data);

    pthread_create(&thread1, NULL, thread_function, &id1);
    pthread_create(&thread2, NULL, thread_function, &id2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Main thread ends, final shared_data = %d\n", shared_data);

    pthread_mutex_destroy(&lock);
    return 0;
}
