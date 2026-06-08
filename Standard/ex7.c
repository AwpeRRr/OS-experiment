#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define SHARED_SIZE 1024

#ifdef _WIN32
static int build_child_command(char *cmdline, size_t size,
                               const char *program,
                               const char *map_name,
                               const char *event_name) {
    int written = snprintf(cmdline, size, "\"%s\" --reader \"%s\" \"%s\"",
                           program, map_name, event_name);
    return written > 0 && (size_t)written < size;
}

static int reader_process(const char *map_name, const char *event_name) {
    HANDLE map_handle;
    HANDLE ready_event;
    char *shared_text;

    ready_event = OpenEventA(SYNCHRONIZE, FALSE, event_name);
    map_handle = OpenFileMappingA(FILE_MAP_READ, FALSE, map_name);
    if (ready_event == NULL || map_handle == NULL) {
        fprintf(stderr, "[reader] failed to open shared objects\n");
        return EXIT_FAILURE;
    }

    WaitForSingleObject(ready_event, INFINITE);

    shared_text = (char *)MapViewOfFile(map_handle, FILE_MAP_READ,
                                        0, 0, SHARED_SIZE);
    if (shared_text == NULL) {
        fprintf(stderr, "[reader] MapViewOfFile failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    printf("[reader] read from shared memory: %s\n", shared_text);

    UnmapViewOfFile(shared_text);
    CloseHandle(map_handle);
    CloseHandle(ready_event);

    return EXIT_SUCCESS;
}

static int writer_process(const char *program) {
    char map_name[128];
    char event_name[128];
    char cmdline[1024];
    HANDLE map_handle;
    HANDLE ready_event;
    char *shared_text;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD exit_code = 1;
    const char *message =
        "This message is written by the writer process into shared memory.";

    snprintf(map_name, sizeof(map_name), "Local\\OSExperiment7Map_%lu",
             (unsigned long)GetCurrentProcessId());
    snprintf(event_name, sizeof(event_name), "Local\\OSExperiment7Ready_%lu",
             (unsigned long)GetCurrentProcessId());

    map_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                    0, SHARED_SIZE, map_name);
    ready_event = CreateEventA(NULL, TRUE, FALSE, event_name);
    if (map_handle == NULL || ready_event == NULL) {
        fprintf(stderr, "[writer] failed to create shared objects\n");
        return EXIT_FAILURE;
    }

    shared_text = (char *)MapViewOfFile(map_handle, FILE_MAP_ALL_ACCESS,
                                        0, 0, SHARED_SIZE);
    if (shared_text == NULL) {
        fprintf(stderr, "[writer] MapViewOfFile failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    if (!build_child_command(cmdline, sizeof(cmdline), program,
                             map_name, event_name)) {
        fprintf(stderr, "[writer] command line is too long\n");
        return EXIT_FAILURE;
    }

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "[writer] CreateProcess failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    strncpy(shared_text, message, SHARED_SIZE - 1);
    shared_text[SHARED_SIZE - 1] = '\0';
    printf("[writer] data has been written to shared memory\n");
    fflush(stdout);
    SetEvent(ready_event);

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    UnmapViewOfFile(shared_text);
    CloseHandle(map_handle);
    CloseHandle(ready_event);

    return exit_code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    if (argc == 4 && strcmp(argv[1], "--reader") == 0) {
        return reader_process(argv[2], argv[3]);
    }

    return writer_process(argv[0]);
}
#else
int main(void) {
    const char *shm_name = "/os_experiment7_shared_memory";
    const char *message =
        "This message is written by the writer process into shared memory.";
    int shm_fd;
    char *shared_text;
    pid_t pid;
    int status;

    shm_unlink(shm_name);
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("[writer] shm_open failed");
        return EXIT_FAILURE;
    }

    if (ftruncate(shm_fd, SHARED_SIZE) < 0) {
        perror("[writer] ftruncate failed");
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }

    shared_text = (char *)mmap(NULL, SHARED_SIZE, PROT_READ | PROT_WRITE,
                               MAP_SHARED, shm_fd, 0);
    if (shared_text == MAP_FAILED) {
        perror("[writer] mmap failed");
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }

    strncpy(shared_text, message, SHARED_SIZE - 1);
    shared_text[SHARED_SIZE - 1] = '\0';
    printf("[writer] data has been written to shared memory\n");
    munmap(shared_text, SHARED_SIZE);
    close(shm_fd);

    pid = fork();
    if (pid < 0) {
        perror("[writer] fork failed");
        shm_unlink(shm_name);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        int reader_fd = shm_open(shm_name, O_RDONLY, 0666);
        char *reader_text;

        if (reader_fd < 0) {
            perror("[reader] shm_open failed");
            return EXIT_FAILURE;
        }

        reader_text = (char *)mmap(NULL, SHARED_SIZE, PROT_READ,
                                   MAP_SHARED, reader_fd, 0);
        if (reader_text == MAP_FAILED) {
            perror("[reader] mmap failed");
            return EXIT_FAILURE;
        }

        printf("[reader] read from shared memory: %s\n", reader_text);
        munmap(reader_text, SHARED_SIZE);
        close(reader_fd);
        return EXIT_SUCCESS;
    }

    waitpid(pid, &status, 0);
    shm_unlink(shm_name);

    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ?
           EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
