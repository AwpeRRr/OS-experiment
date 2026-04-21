#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// 全局共享数据
int shared_data = 0;
// 互斥锁（保护共享数据，防止竞态条件）
pthread_mutex_t lock;

// 线程处理函数
void* thread_function(void* arg) {
    int thread_id = *(int*)arg;
    
    // 加锁后修改全局数据
    pthread_mutex_lock(&lock);
    shared_data += 10;
    printf("[线程 %d] 修改了共享数据，当前值为: %d\n", thread_id, shared_data);
    pthread_mutex_unlock(&lock);
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    int id1 = 1, id2 = 2;

    // 初始化互斥锁
    pthread_mutex_init(&lock, NULL);

    printf("[主进程] 初始共享数据值为: %d\n", shared_data);

    // 创建两个线程
    pthread_create(&thread1, NULL, thread_function, &id1);
    pthread_create(&thread2, NULL, thread_function, &id2);

    // 等待线程执行完毕
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("[主进程] 所有线程执行完毕，最终共享数据值为: %d\n", shared_data);

    // 销毁锁
    pthread_mutex_destroy(&lock);

    return 0;
}