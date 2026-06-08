/*
 * Experiment 5: named pipe communication.
 *
 * Compile on Ubuntu:
 *     gcc standard_5.c -o standard_5
 *
 * Run:
 *     ./standard_5
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define FIFO_NAME "standard_5_fifo"

int main(void)
{
    pid_t pid;
    int status;
    const char *message = "Hello parent, this message is from child.";
    char buffer[128];

    unlink(FIFO_NAME);

    if (mkfifo(FIFO_NAME, 0666) < 0) {
        perror("mkfifo failed");
        return 1;
    }

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        unlink(FIFO_NAME);
        return 1;
    }

    if (pid == 0) {
        int fd;

        /* Child process: open the named pipe and write data. */
        fd = open(FIFO_NAME, O_WRONLY);
        printf("Child process writes to named pipe: %s\n", message);
        write(fd, message, strlen(message) + 1);
        close(fd);
        exit(0);
    } else {
        int fd;

        /* Parent process: open the named pipe and read data. */
        fd = open(FIFO_NAME, O_RDONLY);
        read(fd, buffer, sizeof(buffer));
        printf("Parent process reads from named pipe: %s\n", buffer);
        close(fd);

        wait(&status);
        unlink(FIFO_NAME);
        printf("Parent process removes the named pipe and ends.\n");
    }

    return 0;
}
