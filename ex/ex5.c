#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

#define FIFO_NAME "my_test_fifo"

int main() {
    char write_msg[] = "你好，父进程！这是子进程通过命名管道发送的数据。";
    char read_msg[256];

    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("命名管道创建失败 (可能已存在)");
    }

    pid_t pid = fork(); 

    if (pid < 0) {
        perror("进程创建失败");
        exit(1);
    } else if (pid == 0) {
        
        printf("[子进程] 打开命名管道准备写入...\n");
        int fd = open(FIFO_NAME, O_WRONLY);
        write(fd, write_msg, strlen(write_msg) + 1);
        printf("[子进程] 数据写入完毕。\n");
        close(fd);
        exit(0);
    } else {
        
        printf("[父进程] 打开命名管道准备读取...\n");
        int fd = open(FIFO_NAME, O_RDONLY);
        read(fd, read_msg, sizeof(read_msg));
        printf("[父进程] 成功读取数据: %s\n", read_msg);
        close(fd);
        
        wait(NULL); 
        // 删除命名管道
        unlink(FIFO_NAME);
        printf("[父进程] 通信结束，管道已清理。\n");
    }

    return 0;
}