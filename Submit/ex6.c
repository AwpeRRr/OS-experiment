#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define BUFFER_SIZE 5

// 定义存在于共享内存中的数据结构
typedef struct {
    int buffer[BUFFER_SIZE];
    int in;
    int out;
    sem_t empty; // 空缓冲区数量
    sem_t full;  // 满缓冲区数量
    sem_t mutex; // 互斥锁
} SharedData;

int main() {
    // 申请一块能在父子进程间共享的匿名内存
    SharedData *shared = mmap(NULL, sizeof(SharedData), 
                              PROT_READ | PROT_WRITE, 
                              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    shared->in = 0;
    shared->out = 0;

    // 初始化信号量，第二个参数为 1 表示在进程间共享
    sem_init(&shared->empty, 1, BUFFER_SIZE); 
    sem_init(&shared->full, 1, 0);            
    sem_init(&shared->mutex, 1, 1);           

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        // 子进程：消费者
        for (int i = 0; i < 8; i++) {
            sem_wait(&shared->full);  // 等待有产品
            sem_wait(&shared->mutex); // 进入临界区

            int item = shared->buffer[shared->out];
            printf("[消费者] 消费了产品: %d (位置 %d)\n", item, shared->out);
            shared->out = (shared->out + 1) % BUFFER_SIZE;

            sem_post(&shared->mutex); // 离开临界区
            sem_post(&shared->empty); // 增加空位
            sleep(2); // 模拟消费耗时
        }
        exit(0);
    } else {
        // 父进程：生产者
        for (int i = 1; i <= 8; i++) {
            sem_wait(&shared->empty); // 等待空位
            sem_wait(&shared->mutex); // 进入临界区

            shared->buffer[shared->in] = i;
            printf("[生产者] 生产了产品: %d (位置 %d)\n", i, shared->in);
            shared->in = (shared->in + 1) % BUFFER_SIZE;

            sem_post(&shared->mutex); // 离开临界区
            sem_post(&shared->full);  // 增加产品数量
            sleep(1); // 模拟生产耗时
        }
        wait(NULL);
        
        // 销毁信号量和解除内存映射
        sem_destroy(&shared->empty);
        sem_destroy(&shared->full);
        sem_destroy(&shared->mutex);
        munmap(shared, sizeof(SharedData));
    }

    return 0;
}