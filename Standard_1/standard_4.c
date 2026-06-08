/*
 * Experiment 4: anonymous pipe communication.
 *
 * Compile on Ubuntu:
 *     gcc standard_4.c -o standard_4
 *
 * Run:
 *     ./standard_4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int pipe_fd[2];
    pid_t pid;
    int status;
    char buffer[128];
    const char *message = "Hello child, this message is from parent.";

    if (pipe(pipe_fd) < 0) {
        perror("pipe failed");
        return 1;
    }

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        /* Child process: read data from the pipe. */
        close(pipe_fd[1]);
        read(pipe_fd[0], buffer, sizeof(buffer));
        printf("Child process reads from pipe: %s\n", buffer);
        close(pipe_fd[0]);
        exit(0);
    } else {
        /* Parent process: write data into the pipe. */
        close(pipe_fd[0]);
        printf("Parent process writes to pipe: %s\n", message);
        write(pipe_fd[1], message, strlen(message) + 1);
        close(pipe_fd[1]);

        wait(&status);
        printf("Parent process ends pipe communication.\n");
    }

    return 0;
}
