/*
 * Simple OS experiment: fork a child process to read a file.
 *
 * Compile on Ubuntu:
 *     gcc standard_1.c -o standard_1
 *
 * Run:
 *     ./standard_1 input.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int status;

    if (argc != 2) {
        printf("Usage: %s <file_name>\n", argv[0]);
        return 1;
    }

    printf("Main process starts. PID = %d\n", getpid());

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        FILE *fp;
        char line[256];
        int line_number = 1;

        /* Child process: open the file and read it line by line. */
        printf("Child process starts. PID = %d, Parent PID = %d\n",
               getpid(), getppid());

        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            perror("child process fopen failed");
            exit(1);
        }

        printf("Child process is reading file: %s\n", argv[1]);

        while (fgets(line, sizeof(line), fp) != NULL) {
            printf("Line %d: %s", line_number, line);
            line_number++;
        }

        fclose(fp);

        printf("\nChild process finishes reading the file.\n");
        exit(0);
    } else {
        /* Parent process: wait until the child process finishes. */
        printf("Parent process starts. PID = %d, Child PID = %d\n",
               getpid(), pid);
        printf("Parent process is waiting for the child process...\n");

        wait(&status);

        printf("Parent process detects that the child process has finished.\n");
        printf("Parent process ends.\n");
    }

    return 0;
}
