/*
 * Experiment 7: shared memory communication.
 *
 * Compile on Ubuntu:
 *     gcc standard_7.c -o standard_7
 *
 * Run:
 *     ./standard_7
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SHM_NAME "/standard_7_shared_memory"
#define SHM_SIZE 1024

int main(void)
{
    int shm_fd;
    char *shared_text;
    pid_t pid;
    int status;

    shm_unlink(SHM_NAME);

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("shm_open failed");
        return 1;
    }

    if (ftruncate(shm_fd, SHM_SIZE) < 0) {
        perror("ftruncate failed");
        shm_unlink(SHM_NAME);
        return 1;
    }

    shared_text = mmap(NULL, SHM_SIZE,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED,
                       shm_fd, 0);

    if (shared_text == MAP_FAILED) {
        perror("mmap failed");
        shm_unlink(SHM_NAME);
        return 1;
    }

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        shm_unlink(SHM_NAME);
        return 1;
    }

    if (pid == 0) {
        /* Child process: read data from shared memory. */
        sleep(1);
        printf("Child process reads shared memory: %s\n", shared_text);
        munmap(shared_text, SHM_SIZE);
        close(shm_fd);
        exit(0);
    } else {
        /* Parent process: write data into shared memory. */
        strcpy(shared_text, "This message is written by parent process.");
        printf("Parent process writes data into shared memory.\n");

        wait(&status);

        munmap(shared_text, SHM_SIZE);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        printf("Parent process removes shared memory and ends.\n");
    }

    return 0;
}
