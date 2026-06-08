#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <stdint.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define MESSAGE_SIZE 256

#ifdef _WIN32
static int child_process(HANDLE read_handle, HANDLE write_handle) {
    char buffer[MESSAGE_SIZE];
    DWORD bytes_read = 0;
    DWORD bytes_written = 0;
    const char *reply = "child received the anonymous pipe message";

    if (!ReadFile(read_handle, buffer, sizeof(buffer) - 1, &bytes_read, NULL)) {
        fprintf(stderr, "[child] ReadFile failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }
    buffer[bytes_read] = '\0';
    printf("[child] read from pipe: %s\n", buffer);

    if (!WriteFile(write_handle, reply, (DWORD)strlen(reply) + 1,
                   &bytes_written, NULL)) {
        fprintf(stderr, "[child] WriteFile failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int parent_process(const char *program) {
    SECURITY_ATTRIBUTES sa;
    HANDLE parent_to_child_read = NULL;
    HANDLE parent_to_child_write = NULL;
    HANDLE child_to_parent_read = NULL;
    HANDLE child_to_parent_write = NULL;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmdline[1024];
    char buffer[MESSAGE_SIZE];
    DWORD bytes_read = 0;
    DWORD bytes_written = 0;
    DWORD exit_code = 1;
    const char *message = "parent sends data through an anonymous pipe";

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&parent_to_child_read, &parent_to_child_write, &sa, 0) ||
        !CreatePipe(&child_to_parent_read, &child_to_parent_write, &sa, 0)) {
        fprintf(stderr, "[parent] CreatePipe failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    SetHandleInformation(parent_to_child_write, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(child_to_parent_read, HANDLE_FLAG_INHERIT, 0);

    snprintf(cmdline, sizeof(cmdline), "\"%s\" --child %llu %llu",
             program,
             (unsigned long long)(uintptr_t)parent_to_child_read,
             (unsigned long long)(uintptr_t)child_to_parent_write);

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    printf("[parent] create child process with anonymous pipe handles\n");
    fflush(stdout);
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "[parent] CreateProcess failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    CloseHandle(parent_to_child_read);
    CloseHandle(child_to_parent_write);

    printf("[parent] write to pipe: %s\n", message);
    fflush(stdout);
    WriteFile(parent_to_child_write, message, (DWORD)strlen(message) + 1,
              &bytes_written, NULL);
    CloseHandle(parent_to_child_write);

    if (ReadFile(child_to_parent_read, buffer, sizeof(buffer) - 1,
                 &bytes_read, NULL)) {
        buffer[bytes_read] = '\0';
        printf("[parent] read from pipe: %s\n", buffer);
    }
    CloseHandle(child_to_parent_read);

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return exit_code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    if (argc == 4 && strcmp(argv[1], "--child") == 0) {
        HANDLE read_handle = (HANDLE)(uintptr_t)_strtoui64(argv[2], NULL, 10);
        HANDLE write_handle = (HANDLE)(uintptr_t)_strtoui64(argv[3], NULL, 10);
        return child_process(read_handle, write_handle);
    }

    return parent_process(argv[0]);
}
#else
int main(void) {
    int parent_to_child[2];
    int child_to_parent[2];
    pid_t pid;
    int status;

    if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
        perror("[parent] pipe failed");
        return EXIT_FAILURE;
    }

    pid = fork();
    if (pid < 0) {
        perror("[parent] fork failed");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        char buffer[MESSAGE_SIZE];
        const char *reply = "child received the anonymous pipe message";
        ssize_t n;

        close(parent_to_child[1]);
        close(child_to_parent[0]);

        n = read(parent_to_child[0], buffer, sizeof(buffer) - 1);
        if (n < 0) {
            perror("[child] read failed");
            return EXIT_FAILURE;
        }
        buffer[n] = '\0';
        printf("[child] read from pipe: %s\n", buffer);

        write(child_to_parent[1], reply, strlen(reply) + 1);

        close(parent_to_child[0]);
        close(child_to_parent[1]);
        return EXIT_SUCCESS;
    }

    {
        char buffer[MESSAGE_SIZE];
        const char *message = "parent sends data through an anonymous pipe";
        ssize_t n;

        close(parent_to_child[0]);
        close(child_to_parent[1]);

        printf("[parent] write to pipe: %s\n", message);
        write(parent_to_child[1], message, strlen(message) + 1);
        close(parent_to_child[1]);

        n = read(child_to_parent[0], buffer, sizeof(buffer) - 1);
        if (n >= 0) {
            buffer[n] = '\0';
            printf("[parent] read from pipe: %s\n", buffer);
        }
        close(child_to_parent[0]);

        waitpid(pid, &status, 0);
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ?
           EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
