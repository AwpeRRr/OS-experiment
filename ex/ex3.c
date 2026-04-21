#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// 信号处理函数
void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        printf("[父进程] 接收到了子进程发送的 SIGUSR1 信号！进行相应处理...\n");
    }
}

int main() {
    // 注册信号处理函数
    signal(SIGUSR1, signal_handler);

    pid_t pid = fork();

    if (pid < 0) {
        perror("进程创建失败");
        exit(1);
    } else if (pid == 0) {
        // 子进程区域
        printf("[子进程] 准备向父进程发送信号...\n");
        sleep(1); // 模拟耗时操作，确保父进程已经准备好
        printf("[子进程] 发送信号 SIGUSR1。\n");
        kill(getppid(), SIGUSR1); // 向父进程发送信号
        exit(0);
    } else {
        // 父进程区域
        printf("[父进程] 正在等待子进程的信号...\n");
        wait(NULL); // 等待子进程结束
        printf("[父进程] 退出。\n");
    }

    return 0;
}