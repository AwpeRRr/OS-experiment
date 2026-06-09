#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid;
    char write_msg[] = "你好，Child！这是来自 Parent 的匿名管道数据。";

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // 子进程中执行 Child 程序
        close(pipefd[1]);

        char fd_arg[16];
        sprintf(fd_arg, "%d", pipefd[0]);

        execl("./Child", "Child", fd_arg, NULL);

        perror("execl failed");
        exit(1);
    } else {
        // Parent 程序
        close(pipefd[0]);

        printf("[Parent] 正在向匿名管道写入数据...\n");
        write(pipefd[1], write_msg, strlen(write_msg) + 1);

        close(pipefd[1]);
        wait(NULL);

        printf("[Parent] Child 已结束，通信完成。\n");
    }

    return 0;
}
