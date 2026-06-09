#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int pipefd[2]; 
    pid_t pid;
    char write_msg[] = "你好，子进程！这是来自父进程的数据。";
    char read_msg[100];

    
    if (pipe(pipefd) == -1) {
        perror("管道创建失败");
        exit(1);
    }

    pid = fork();

    if (pid < 0) {
        perror("进程创建失败");
        exit(1);
    } else if (pid == 0) {
        close(pipefd[1]); 
        
        printf("[子进程] 等待读取管道数据...\n");
        read(pipefd[0], read_msg, sizeof(read_msg));
        printf("[子进程] 成功从管道读取到: %s\n", read_msg);
        
        close(pipefd[0]); 
        exit(0);
    } else {
        close(pipefd[0]); 
        
        printf("[父进程] 正在向管道写入数据...\n");
        write(pipefd[1], write_msg, strlen(write_msg) + 1);
        
        close(pipefd[1]); 
        wait(NULL);       
        printf("[父进程] 通信结束。\n");
    }

    return 0;
}