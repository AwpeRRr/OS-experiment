#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

typedef struct ThreadData {
    int id;
    int private_value;
} ThreadData;

static int shared_data = 0;
static ThreadData thread_data[2] = {{1, 100}, {2, 200}};
static ThreadData *shared_pointer = NULL;

#ifdef _WIN32
static CRITICAL_SECTION data_lock;
#else
static pthread_mutex_t data_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static void lock_data(void) {
#ifdef _WIN32
    EnterCriticalSection(&data_lock);
#else
    pthread_mutex_lock(&data_lock);
#endif
}

static void unlock_data(void) {
#ifdef _WIN32
    LeaveCriticalSection(&data_lock);
#else
    pthread_mutex_unlock(&data_lock);
#endif
}

static void thread_body(ThreadData *self) {
    lock_data();

    shared_data += self->id * 10;
    printf("[thread %d] changed shared_data to %d\n",
           self->id, shared_data);

    if (self->id == 1 && shared_pointer != NULL) {
        shared_pointer->private_value += 50;
        printf("[thread %d] changed thread 2 data through global pointer, "
               "thread2.private_value = %d\n",
               self->id, shared_pointer->private_value);
    } else if (self->id == 2) {
        self->private_value += 30;
        printf("[thread %d] changed its own data, private_value = %d\n",
               self->id, self->private_value);
    }

    unlock_data();
}

#ifdef _WIN32
static DWORD WINAPI thread_entry(LPVOID arg) {
    thread_body((ThreadData *)arg);
    return 0;
}
#else
static void *thread_entry(void *arg) {
    thread_body((ThreadData *)arg);
    return NULL;
}
#endif

int main(void) {
    shared_pointer = &thread_data[1];

    printf("[main] initial shared_data = %d\n", shared_data);
    printf("[main] initial thread1.private_value = %d, "
           "thread2.private_value = %d\n",
           thread_data[0].private_value, thread_data[1].private_value);

#ifdef _WIN32
    {
        HANDLE threads[2];

        InitializeCriticalSection(&data_lock);

        threads[0] = CreateThread(NULL, 0, thread_entry, &thread_data[0], 0, NULL);
        threads[1] = CreateThread(NULL, 0, thread_entry, &thread_data[1], 0, NULL);
        if (threads[0] == NULL || threads[1] == NULL) {
            fprintf(stderr, "[main] CreateThread failed\n");
            return EXIT_FAILURE;
        }

        WaitForMultipleObjects(2, threads, TRUE, INFINITE);
        CloseHandle(threads[0]);
        CloseHandle(threads[1]);

        DeleteCriticalSection(&data_lock);
    }
#else
    {
        pthread_t threads[2];

        if (pthread_create(&threads[0], NULL, thread_entry, &thread_data[0]) != 0 ||
            pthread_create(&threads[1], NULL, thread_entry, &thread_data[1]) != 0) {
            perror("[main] pthread_create failed");
            return EXIT_FAILURE;
        }

        pthread_join(threads[0], NULL);
        pthread_join(threads[1], NULL);
        pthread_mutex_destroy(&data_lock);
    }
#endif

    printf("[main] final shared_data = %d\n", shared_data);
    printf("[main] final thread1.private_value = %d, "
           "thread2.private_value = %d\n",
           thread_data[0].private_value, thread_data[1].private_value);
    printf("[main] result: threads in one process can access process data, "
           "and can also modify another thread's data when its address is shared.\n");

    return EXIT_SUCCESS;
}
