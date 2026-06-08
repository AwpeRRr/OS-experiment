#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define BUFFER_SIZE 5
#define PRODUCT_COUNT 10

typedef struct SharedBuffer {
    int buffer[BUFFER_SIZE];
    int in;
    int out;
#ifndef _WIN32
    sem_t empty;
    sem_t full;
    sem_t mutex;
#endif
} SharedBuffer;

#ifdef _WIN32
static int wait_sem(HANDLE semaphore, const char *name) {
    if (WaitForSingleObject(semaphore, INFINITE) != WAIT_OBJECT_0) {
        fprintf(stderr, "WaitForSingleObject(%s) failed, error: %lu\n",
                name, (unsigned long)GetLastError());
        return 0;
    }
    return 1;
}

static void post_sem(HANDLE semaphore) {
    ReleaseSemaphore(semaphore, 1, NULL);
}

static void consume_items(SharedBuffer *shared,
                          HANDLE empty_sem,
                          HANDLE full_sem,
                          HANDLE mutex_sem) {
    int i;

    for (i = 0; i < PRODUCT_COUNT; i++) {
        int item;
        int pos;

        wait_sem(full_sem, "full");
        wait_sem(mutex_sem, "mutex");

        pos = shared->out;
        item = shared->buffer[pos];
        shared->out = (shared->out + 1) % BUFFER_SIZE;
        printf("[consumer] consume item %d from slot %d\n", item, pos);
        fflush(stdout);

        post_sem(mutex_sem);
        post_sem(empty_sem);
        Sleep(700);
    }
}

static void produce_items(SharedBuffer *shared,
                          HANDLE empty_sem,
                          HANDLE full_sem,
                          HANDLE mutex_sem) {
    int i;

    for (i = 1; i <= PRODUCT_COUNT; i++) {
        int pos;

        wait_sem(empty_sem, "empty");
        wait_sem(mutex_sem, "mutex");

        pos = shared->in;
        shared->buffer[pos] = i;
        shared->in = (shared->in + 1) % BUFFER_SIZE;
        printf("[producer] produce item %d into slot %d\n", i, pos);
        fflush(stdout);

        post_sem(mutex_sem);
        post_sem(full_sem);
        Sleep(400);
    }
}

static int build_child_command(char *cmdline, size_t size,
                               const char *program,
                               const char *map_name,
                               const char *empty_name,
                               const char *full_name,
                               const char *mutex_name) {
    int written = snprintf(cmdline, size,
                           "\"%s\" --consumer \"%s\" \"%s\" \"%s\" \"%s\"",
                           program, map_name, empty_name, full_name, mutex_name);
    return written > 0 && (size_t)written < size;
}

static int child_process(const char *map_name,
                         const char *empty_name,
                         const char *full_name,
                         const char *mutex_name) {
    HANDLE map_handle;
    HANDLE empty_sem;
    HANDLE full_sem;
    HANDLE mutex_sem;
    SharedBuffer *shared;

    map_handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, map_name);
    empty_sem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, empty_name);
    full_sem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, full_name);
    mutex_sem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, mutex_name);

    if (map_handle == NULL || empty_sem == NULL ||
        full_sem == NULL || mutex_sem == NULL) {
        fprintf(stderr, "[consumer] failed to open shared objects\n");
        return EXIT_FAILURE;
    }

    shared = (SharedBuffer *)MapViewOfFile(map_handle, FILE_MAP_ALL_ACCESS,
                                           0, 0, sizeof(SharedBuffer));
    if (shared == NULL) {
        fprintf(stderr, "[consumer] MapViewOfFile failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    consume_items(shared, empty_sem, full_sem, mutex_sem);

    UnmapViewOfFile(shared);
    CloseHandle(map_handle);
    CloseHandle(empty_sem);
    CloseHandle(full_sem);
    CloseHandle(mutex_sem);

    return EXIT_SUCCESS;
}

static int parent_process(const char *program) {
    char map_name[128];
    char empty_name[128];
    char full_name[128];
    char mutex_name[128];
    char cmdline[1024];
    HANDLE map_handle;
    HANDLE empty_sem;
    HANDLE full_sem;
    HANDLE mutex_sem;
    SharedBuffer *shared;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD exit_code = 1;

    snprintf(map_name, sizeof(map_name), "Local\\OSExperiment6Map_%lu",
             (unsigned long)GetCurrentProcessId());
    snprintf(empty_name, sizeof(empty_name), "Local\\OSExperiment6Empty_%lu",
             (unsigned long)GetCurrentProcessId());
    snprintf(full_name, sizeof(full_name), "Local\\OSExperiment6Full_%lu",
             (unsigned long)GetCurrentProcessId());
    snprintf(mutex_name, sizeof(mutex_name), "Local\\OSExperiment6Mutex_%lu",
             (unsigned long)GetCurrentProcessId());

    map_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                    0, sizeof(SharedBuffer), map_name);
    empty_sem = CreateSemaphoreA(NULL, BUFFER_SIZE, BUFFER_SIZE, empty_name);
    full_sem = CreateSemaphoreA(NULL, 0, BUFFER_SIZE, full_name);
    mutex_sem = CreateSemaphoreA(NULL, 1, 1, mutex_name);

    if (map_handle == NULL || empty_sem == NULL ||
        full_sem == NULL || mutex_sem == NULL) {
        fprintf(stderr, "[producer] failed to create shared objects\n");
        return EXIT_FAILURE;
    }

    shared = (SharedBuffer *)MapViewOfFile(map_handle, FILE_MAP_ALL_ACCESS,
                                           0, 0, sizeof(SharedBuffer));
    if (shared == NULL) {
        fprintf(stderr, "[producer] MapViewOfFile failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    memset(shared, 0, sizeof(*shared));

    if (!build_child_command(cmdline, sizeof(cmdline), program, map_name,
                             empty_name, full_name, mutex_name)) {
        fprintf(stderr, "[producer] command line is too long\n");
        return EXIT_FAILURE;
    }

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    printf("[producer] create consumer process\n");
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "[producer] CreateProcess failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    produce_items(shared, empty_sem, full_sem, mutex_sem);

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    UnmapViewOfFile(shared);
    CloseHandle(map_handle);
    CloseHandle(empty_sem);
    CloseHandle(full_sem);
    CloseHandle(mutex_sem);

    return exit_code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    if (argc == 6 && strcmp(argv[1], "--consumer") == 0) {
        return child_process(argv[2], argv[3], argv[4], argv[5]);
    }

    return parent_process(argv[0]);
}
#else
static void consume_items(SharedBuffer *shared) {
    int i;

    for (i = 0; i < PRODUCT_COUNT; i++) {
        int item;
        int pos;

        sem_wait(&shared->full);
        sem_wait(&shared->mutex);

        pos = shared->out;
        item = shared->buffer[pos];
        shared->out = (shared->out + 1) % BUFFER_SIZE;
        printf("[consumer] consume item %d from slot %d\n", item, pos);
        fflush(stdout);

        sem_post(&shared->mutex);
        sem_post(&shared->empty);
        usleep(700000);
    }
}

static void produce_items(SharedBuffer *shared) {
    int i;

    for (i = 1; i <= PRODUCT_COUNT; i++) {
        int pos;

        sem_wait(&shared->empty);
        sem_wait(&shared->mutex);

        pos = shared->in;
        shared->buffer[pos] = i;
        shared->in = (shared->in + 1) % BUFFER_SIZE;
        printf("[producer] produce item %d into slot %d\n", i, pos);
        fflush(stdout);

        sem_post(&shared->mutex);
        sem_post(&shared->full);
        usleep(400000);
    }
}

int main(void) {
    SharedBuffer *shared;
    pid_t pid;
    int status;

    shared = (SharedBuffer *)mmap(NULL, sizeof(SharedBuffer),
                                  PROT_READ | PROT_WRITE,
                                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared == MAP_FAILED) {
        perror("[main] mmap failed");
        return EXIT_FAILURE;
    }

    memset(shared, 0, sizeof(*shared));
    sem_init(&shared->empty, 1, BUFFER_SIZE);
    sem_init(&shared->full, 1, 0);
    sem_init(&shared->mutex, 1, 1);

    pid = fork();
    if (pid < 0) {
        perror("[main] fork failed");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        consume_items(shared);
        return EXIT_SUCCESS;
    }

    produce_items(shared);
    waitpid(pid, &status, 0);

    sem_destroy(&shared->empty);
    sem_destroy(&shared->full);
    sem_destroy(&shared->mutex);
    munmap(shared, sizeof(SharedBuffer));

    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ?
           EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
