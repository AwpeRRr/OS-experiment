/*
 * Experiment 6: producer-consumer problem.
 *
 * Compile on Ubuntu:
 *     gcc standard_6.c -o standard_6 -pthread
 *
 * Run:
 *     ./standard_6
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define BUFFER_SIZE 5
#define ITEM_COUNT 10

struct SharedBuffer {
    int buffer[BUFFER_SIZE];
    int in;
    int out;
    sem_t empty;
    sem_t full;
    sem_t mutex;
};

int main(void)
{
    struct SharedBuffer *shared;
    pid_t pid;
    int status;
    int i;

    shared = mmap(NULL, sizeof(struct SharedBuffer),
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS,
                  -1, 0);

    if (shared == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    shared->in = 0;
    shared->out = 0;

    sem_init(&shared->empty, 1, BUFFER_SIZE);
    sem_init(&shared->full, 1, 0);
    sem_init(&shared->mutex, 1, 1);

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        /* Child process: consumer. */
        for (i = 0; i < ITEM_COUNT; i++) {
            int item;
            int pos;

            sem_wait(&shared->full);
            sem_wait(&shared->mutex);

            pos = shared->out;
            item = shared->buffer[pos];
            shared->out = (shared->out + 1) % BUFFER_SIZE;
            printf("Consumer gets item %d from buffer[%d]\n", item, pos);

            sem_post(&shared->mutex);
            sem_post(&shared->empty);

            sleep(1);
        }

        exit(0);
    } else {
        /* Parent process: producer. */
        for (i = 1; i <= ITEM_COUNT; i++) {
            int pos;

            sem_wait(&shared->empty);
            sem_wait(&shared->mutex);

            pos = shared->in;
            shared->buffer[pos] = i;
            shared->in = (shared->in + 1) % BUFFER_SIZE;
            printf("Producer puts item %d into buffer[%d]\n", i, pos);

            sem_post(&shared->mutex);
            sem_post(&shared->full);

            sleep(1);
        }

        wait(&status);

        sem_destroy(&shared->empty);
        sem_destroy(&shared->full);
        sem_destroy(&shared->mutex);
        munmap(shared, sizeof(struct SharedBuffer));
    }

    return 0;
}
