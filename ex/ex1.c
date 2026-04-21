#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork(); // 创建子进程

    if (pid < 0) {
        perror("进程创建失败");
        exit(1);
    } else if (pid == 0) {
        // 子进程区域
        printf("[子进程] 正在读取文件...\n");
        FILE *file = fopen("test.txt", "r");
        if (file == NULL) {
            perror("[子进程] 文件打开失败，请确保test.txt存在");
            exit(1);
        }
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            printf("[子进程] 读取内容: %s", buffer);
        }
        printf("\n");
        fclose(file);
        printf("[子进程] 文件读取完毕，即将退出。\n");
        exit(0);
    } else {
        // 父进程区域
        printf("[父进程] 等待子进程完成任务...\n");
        wait(NULL); // 等待子进程结束 [cite: 24, 25]
        printf("[父进程] 子进程已结束，父进程继续执行并退出。\n");
    }

    return 0;
}