#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// 全局共享数据：两个线程都可以直接访问并修改它。
int shared_data = 0;

// 线程自己的数据。实验要求演示“一个线程更改另一个线程的数据”，
// 所以主线程把两个线程的数据地址保存到全局指针中。
typedef struct {
    int thread_id;
    int private_data;
} ThreadData;

ThreadData *thread1_data = NULL;
ThreadData *thread2_data = NULL;

// 互斥锁用于保护共享数据和通过全局指针访问的线程数据。
pthread_mutex_t lock;

void* thread_function(void* arg) {
    ThreadData *self = (ThreadData*)arg;

    pthread_mutex_lock(&lock);

    shared_data += 10;
    printf("[线程 %d] 修改全局共享数据，当前 shared_data = %d\n",
           self->thread_id, shared_data);

    if (self->thread_id == 1 && thread2_data != NULL) {
        thread2_data->private_data += 100;
        printf("[线程 1] 通过全局指针修改线程 2 的数据，thread2.private_data = %d\n",
               thread2_data->private_data);
    } else if (self->thread_id == 2 && thread1_data != NULL) {
        thread1_data->private_data += 200;
        printf("[线程 2] 通过全局指针修改线程 1 的数据，thread1.private_data = %d\n",
               thread1_data->private_data);
    }

    pthread_mutex_unlock(&lock);

    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    ThreadData data1 = {1, 1000};
    ThreadData data2 = {2, 2000};

    thread1_data = &data1;
    thread2_data = &data2;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("pthread_mutex_init failed");
        return 1;
    }

    printf("[主进程] 初始 shared_data = %d\n", shared_data);
    printf("[主进程] 初始 thread1.private_data = %d, thread2.private_data = %d\n",
           data1.private_data, data2.private_data);

    if (pthread_create(&thread1, NULL, thread_function, &data1) != 0) {
        perror("pthread_create thread1 failed");
        pthread_mutex_destroy(&lock);
        return 1;
    }

    if (pthread_create(&thread2, NULL, thread_function, &data2) != 0) {
        perror("pthread_create thread2 failed");
        pthread_join(thread1, NULL);
        pthread_mutex_destroy(&lock);
        return 1;
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("[主进程] 所有线程执行完毕，最终 shared_data = %d\n", shared_data);
    printf("[主进程] 最终 thread1.private_data = %d, thread2.private_data = %d\n",
           data1.private_data, data2.private_data);

    pthread_mutex_destroy(&lock);

    return 0;
}
