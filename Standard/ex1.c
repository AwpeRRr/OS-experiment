#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define BUFFER_SIZE 256

static unsigned long current_process_id(void) {
#ifdef _WIN32
    return (unsigned long)GetCurrentProcessId();
#else
    return (unsigned long)getpid();
#endif
}

static int read_file_in_child(const char *file_name) {
    FILE *fp;
    char buffer[BUFFER_SIZE];
    unsigned long line_count = 0;

    printf("[child  %lu] open file: %s\n", current_process_id(), file_name);

    fp = fopen(file_name, "r");
    if (fp == NULL) {
        perror("[child] fopen failed");
        return EXIT_FAILURE;
    }

    printf("[child  %lu] file content begins\n", current_process_id());
    printf("----------------------------------------\n");

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        fputs(buffer, stdout);
        line_count++;
    }

    if (ferror(fp)) {
        perror("[child] fgets failed");
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (line_count == 0) {
        printf("(empty file)\n");
    }

    printf("\n----------------------------------------\n");
    printf("[child  %lu] finished reading, lines: %lu\n",
           current_process_id(), line_count);

    fclose(fp);
    return EXIT_SUCCESS;
}

#ifdef _WIN32
static int append_char(char *dst, size_t size, size_t *pos, char ch) {
    if (*pos + 1 >= size) {
        return 0;
    }
    dst[*pos] = ch;
    (*pos)++;
    dst[*pos] = '\0';
    return 1;
}

static int append_repeated_char(char *dst, size_t size, size_t *pos,
                                char ch, size_t count) {
    size_t i;

    for (i = 0; i < count; i++) {
        if (!append_char(dst, size, pos, ch)) {
            return 0;
        }
    }

    return 1;
}

static int append_quoted_arg(char *dst, size_t size, size_t *pos,
                             const char *arg) {
    size_t slash_count = 0;
    const char *p;

    if (!append_char(dst, size, pos, '"')) {
        return 0;
    }

    for (p = arg; *p != '\0'; p++) {
        if (*p == '\\') {
            slash_count++;
            continue;
        }

        if (*p == '"') {
            if (!append_repeated_char(dst, size, pos, '\\', slash_count * 2 + 1)) {
                return 0;
            }
            slash_count = 0;
            if (!append_char(dst, size, pos, '"')) {
                return 0;
            }
            continue;
        }

        if (!append_repeated_char(dst, size, pos, '\\', slash_count)) {
            return 0;
        }
        slash_count = 0;

        if (!append_char(dst, size, pos, *p)) {
            return 0;
        }
    }

    if (!append_repeated_char(dst, size, pos, '\\', slash_count * 2)) {
        return 0;
    }

    return append_char(dst, size, pos, '"');
}

static int append_space(char *dst, size_t size, size_t *pos) {
    return append_char(dst, size, pos, ' ');
}

static int build_child_command(char *cmdline, size_t size,
                               const char *exe_path, const char *file_name) {
    size_t pos = 0;

    cmdline[0] = '\0';

    return append_quoted_arg(cmdline, size, &pos, exe_path) &&
           append_space(cmdline, size, &pos) &&
           append_quoted_arg(cmdline, size, &pos, "--child") &&
           append_space(cmdline, size, &pos) &&
           append_quoted_arg(cmdline, size, &pos, file_name);
}

static int run_parent_process(const char *file_name) {
    char exe_path[MAX_PATH];
    char cmdline[4096];
    DWORD len;
    DWORD exit_code = 1;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    len = GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    if (len == 0 || len >= sizeof(exe_path)) {
        fprintf(stderr, "[parent] failed to get executable path\n");
        return EXIT_FAILURE;
    }

    if (!build_child_command(cmdline, sizeof(cmdline), exe_path, file_name)) {
        fprintf(stderr, "[parent] command line is too long\n");
        return EXIT_FAILURE;
    }

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    printf("[parent %lu] create child process\n", current_process_id());
    fflush(stdout);

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "[parent] CreateProcess failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    printf("[parent %lu] waiting for child process %lu\n",
           current_process_id(), (unsigned long)pi.dwProcessId);
    fflush(stdout);

    WaitForSingleObject(pi.hProcess, INFINITE);

    if (!GetExitCodeProcess(pi.hProcess, &exit_code)) {
        fprintf(stderr, "[parent] GetExitCodeProcess failed, error: %lu\n",
                (unsigned long)GetLastError());
        exit_code = 1;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    printf("[parent %lu] child finished, exit code: %lu\n",
           current_process_id(), (unsigned long)exit_code);
    printf("[parent %lu] parent continues and exits\n", current_process_id());

    return exit_code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
#else
static int run_parent_process(const char *file_name) {
    pid_t pid;
    int status;

    printf("[parent %lu] create child process\n", current_process_id());
    fflush(stdout);

    pid = fork();
    if (pid < 0) {
        perror("[parent] fork failed");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        return read_file_in_child(file_name);
    }

    printf("[parent %lu] waiting for child process %lu\n",
           current_process_id(), (unsigned long)pid);
    fflush(stdout);

    if (waitpid(pid, &status, 0) < 0) {
        perror("[parent] waitpid failed");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        int child_code = WEXITSTATUS(status);
        printf("[parent %lu] child finished, exit code: %d\n",
               current_process_id(), child_code);
        printf("[parent %lu] parent continues and exits\n", current_process_id());
        return child_code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (WIFSIGNALED(status)) {
        printf("[parent %lu] child was terminated by signal: %d\n",
               current_process_id(), WTERMSIG(status));
    }

    return EXIT_FAILURE;
}
#endif

static void print_usage(const char *program_name) {
    printf("Usage: %s [file_name]\n", program_name);
    printf("If file_name is omitted, the child process reads test.txt.\n");
}

int main(int argc, char *argv[]) {
    const char *file_name = "test.txt";

    if (argc >= 2 &&
        (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (argc >= 2 && strcmp(argv[1], "--child") == 0) {
        if (argc >= 3) {
            file_name = argv[2];
        }
        return read_file_in_child(file_name);
    }

    if (argc >= 2) {
        file_name = argv[1];
    }

    return run_parent_process(file_name);
}
