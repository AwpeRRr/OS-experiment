#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define MESSAGE_SIZE 256

#ifdef _WIN32
static int build_child_command(char *cmdline, size_t size,
                               const char *program, const char *pipe_name) {
    int written = snprintf(cmdline, size, "\"%s\" --child \"%s\"",
                           program, pipe_name);
    return written > 0 && (size_t)written < size;
}

static int child_process(const char *pipe_name) {
    HANDLE pipe_handle;
    DWORD bytes_written = 0;
    const char *message = "child writes data through a named pipe";

    Sleep(500);
    pipe_handle = CreateFileA(pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                              0, NULL);
    if (pipe_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[child] CreateFile(named pipe) failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    printf("[child] write to named pipe: %s\n", message);
    WriteFile(pipe_handle, message, (DWORD)strlen(message) + 1,
              &bytes_written, NULL);
    CloseHandle(pipe_handle);

    return EXIT_SUCCESS;
}

static int parent_process(const char *program) {
    char pipe_name[128];
    char cmdline[1024];
    char buffer[MESSAGE_SIZE];
    HANDLE pipe_handle;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD bytes_read = 0;
    DWORD exit_code = 1;

    snprintf(pipe_name, sizeof(pipe_name),
             "\\\\.\\pipe\\OSExperiment5Pipe_%lu",
             (unsigned long)GetCurrentProcessId());

    pipe_handle = CreateNamedPipeA(pipe_name, PIPE_ACCESS_INBOUND,
                                   PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE |
                                   PIPE_WAIT,
                                   1, MESSAGE_SIZE, MESSAGE_SIZE, 0, NULL);
    if (pipe_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[parent] CreateNamedPipe failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    if (!build_child_command(cmdline, sizeof(cmdline), program, pipe_name)) {
        fprintf(stderr, "[parent] command line is too long\n");
        CloseHandle(pipe_handle);
        return EXIT_FAILURE;
    }

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    printf("[parent] create named pipe and child process\n");
    fflush(stdout);
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "[parent] CreateProcess failed, error: %lu\n",
                (unsigned long)GetLastError());
        CloseHandle(pipe_handle);
        return EXIT_FAILURE;
    }

    if (!ConnectNamedPipe(pipe_handle, NULL) &&
        GetLastError() != ERROR_PIPE_CONNECTED) {
        fprintf(stderr, "[parent] ConnectNamedPipe failed, error: %lu\n",
                (unsigned long)GetLastError());
    }

    if (ReadFile(pipe_handle, buffer, sizeof(buffer) - 1, &bytes_read, NULL)) {
        buffer[bytes_read] = '\0';
        printf("[parent] read from named pipe: %s\n", buffer);
    }

    DisconnectNamedPipe(pipe_handle);
    CloseHandle(pipe_handle);

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return exit_code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    if (argc >= 3 && strcmp(argv[1], "--child") == 0) {
        return child_process(argv[2]);
    }

    return parent_process(argv[0]);
}
#else
int main(void) {
    const char *fifo_name = "os_experiment5_fifo";
    const char *message = "child writes data through a named pipe";
    pid_t pid;
    int status;

    if (mkfifo(fifo_name, 0666) < 0 && errno != EEXIST) {
        perror("[parent] mkfifo failed");
        return EXIT_FAILURE;
    }

    pid = fork();
    if (pid < 0) {
        perror("[parent] fork failed");
        unlink(fifo_name);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        int fd = open(fifo_name, O_WRONLY);
        if (fd < 0) {
            perror("[child] open fifo for write failed");
            return EXIT_FAILURE;
        }

        printf("[child] write to named pipe: %s\n", message);
        write(fd, message, strlen(message) + 1);
        close(fd);
        return EXIT_SUCCESS;
    }

    {
        char buffer[MESSAGE_SIZE];
        int fd = open(fifo_name, O_RDONLY);
        ssize_t n;

        if (fd < 0) {
            perror("[parent] open fifo for read failed");
            unlink(fifo_name);
            return EXIT_FAILURE;
        }

        n = read(fd, buffer, sizeof(buffer) - 1);
        if (n >= 0) {
            buffer[n] = '\0';
            printf("[parent] read from named pipe: %s\n", buffer);
        }
        close(fd);
        waitpid(pid, &status, 0);
    }

    unlink(fifo_name);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ?
           EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
