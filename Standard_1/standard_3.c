/*
 * Experiment 3: process communication with signal.
 *
 * Compile on Ubuntu:
 *     gcc standard_3.c -o standard_3
 *
 * Run:
 *     ./standard_3
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

volatile sig_atomic_t got_signal = 0;

void signal_handler(int sig)
{
    if (sig == SIGUSR1) {
        got_signal = 1;
    }
}

int main(void)
{
    pid_t pid;
    int status;

    signal(SIGUSR1, signal_handler);

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        /* Child process: send a signal to the parent process. */
        printf("Child process starts. PID = %d\n", getpid());
        sleep(1);
        printf("Child process sends SIGUSR1 to parent process.\n");
        kill(getppid(), SIGUSR1);
        exit(0);
    } else {
        /* Parent process: wait for the signal from the child process. */
        printf("Parent process starts. PID = %d\n", getpid());
        printf("Parent process is waiting for SIGUSR1...\n");

        while (!got_signal) {
            pause();
        }

        printf("Parent process received SIGUSR1 from child process.\n");
        wait(&status);
        printf("Parent process ends.\n");
    }

    return 0;
}
