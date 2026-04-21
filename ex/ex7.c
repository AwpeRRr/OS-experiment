#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>

#define SHM_NAME "/my_shm_test"
#define SHM_SIZE 1024

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid > 0) {
        // 父进程：写者进程
        // 创建共享主存
        int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        ftruncate(shm_fd, SHM_SIZE); // 设置共享内存大小
        
        // 映射到虚拟地址空间
        char *ptr = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
        
        const char *msg = "你好，这是通过共享主存传递的机密信息！";
        sprintf(ptr, "%s", msg); // 写入数据
        printf("[写者进程] 数据已写入共享主存。\n");
        
        wait(NULL); // 等待读者读取完毕
        
        // 清理共享主存
        shm_unlink(SHM_NAME);
    } else {
        // 子进程：读者进程
        sleep(1); // 确保写者先创建并写入

        // 打开已存在的共享主存
        int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
        // 映射到虚拟地址空间
        char *ptr = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
        
        printf("[读者进程] 从共享主存中读取到: %s\n", ptr);
        exit(0);
    }

    return 0;
}