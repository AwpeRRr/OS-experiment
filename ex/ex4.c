#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int pipefd[2]; // pipefd[0]是读端，pipefd[1]是写端
    pid_t pid;
    char write_msg[] = "你好，子进程！这是来自父进程的数据。";
    char read_msg[100];

    // 创建匿名管道
    if (pipe(pipefd) == -1) {
        perror("管道创建失败");
        exit(1);
    }

    pid = fork();

    if (pid < 0) {
        perror("进程创建失败");
        exit(1);
    } else if (pid == 0) {
        // 子进程区域 (读取数据)
        close(pipefd[1]); // 关闭写端
        
        printf("[子进程] 等待读取管道数据...\n");
        read(pipefd[0], read_msg, sizeof(read_msg));
        printf("[子进程] 成功从管道读取到: %s\n", read_msg);
        
        close(pipefd[0]); // 读取完毕，关闭读端
        exit(0);
    } else {
        // 父进程区域 (写入数据)
        close(pipefd[0]); // 关闭读端
        
        printf("[父进程] 正在向管道写入数据...\n");
        write(pipefd[1], write_msg, strlen(write_msg) + 1);
        
        close(pipefd[1]); // 写入完毕，关闭写端
        wait(NULL);       // 等待子进程结束
        printf("[父进程] 通信结束。\n");
    }

    return 0;
}