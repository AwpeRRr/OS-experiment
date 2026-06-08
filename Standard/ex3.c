#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef _WIN32
static int build_child_command(char *cmdline, size_t size,
                               const char *program, const char *event_name) {
    int written = snprintf(cmdline, size, "\"%s\" --child \"%s\"",
                           program, event_name);
    return written > 0 && (size_t)written < size;
}

static int child_process(const char *event_name) {
    HANDLE event_handle;

    printf("[child] open named event: %s\n", event_name);
    event_handle = OpenEventA(EVENT_MODIFY_STATE, FALSE, event_name);
    if (event_handle == NULL) {
        fprintf(stderr, "[child] OpenEvent failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    Sleep(1000);
    printf("[child] send event signal to parent\n");
    if (!SetEvent(event_handle)) {
        fprintf(stderr, "[child] SetEvent failed, error: %lu\n",
                (unsigned long)GetLastError());
        CloseHandle(event_handle);
        return EXIT_FAILURE;
    }

    CloseHandle(event_handle);
    return EXIT_SUCCESS;
}

static int parent_process(const char *program) {
    char event_name[128];
    char cmdline[1024];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    HANDLE event_handle;
    DWORD exit_code = 1;

    snprintf(event_name, sizeof(event_name), "Local\\OSExperiment3Event_%lu",
             (unsigned long)GetCurrentProcessId());

    event_handle = CreateEventA(NULL, FALSE, FALSE, event_name);
    if (event_handle == NULL) {
        fprintf(stderr, "[parent] CreateEvent failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    if (!build_child_command(cmdline, sizeof(cmdline), program, event_name)) {
        fprintf(stderr, "[parent] command line is too long\n");
        CloseHandle(event_handle);
        return EXIT_FAILURE;
    }

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    printf("[parent] create child process and wait for named event\n");
    fflush(stdout);
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "[parent] CreateProcess failed, error: %lu\n",
                (unsigned long)GetLastError());
        CloseHandle(event_handle);
        return EXIT_FAILURE;
    }

    WaitForSingleObject(event_handle, INFINITE);
    printf("[parent] received event signal from child, handling it now\n");

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(event_handle);

    return exit_code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    if (argc >= 3 && strcmp(argv[1], "--child") == 0) {
        return child_process(argv[2]);
    }

    return parent_process(argv[0]);
}
#else
static volatile sig_atomic_t signal_received = 0;

static void signal_handler(int signo) {
    if (signo == SIGUSR1) {
        signal_received = 1;
    }
}

int main(void) {
    pid_t pid;
    sigset_t block_mask;
    sigset_t old_mask;
    struct sigaction sa;
    int status;

    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &block_mask, &old_mask) < 0) {
        perror("[parent] sigprocmask failed");
        return EXIT_FAILURE;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("[parent] sigaction failed");
        return EXIT_FAILURE;
    }

    pid = fork();
    if (pid < 0) {
        perror("[parent] fork failed");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        printf("[child] prepare to send SIGUSR1 to parent %lu\n",
               (unsigned long)getppid());
        sleep(1);
        kill(getppid(), SIGUSR1);
        printf("[child] SIGUSR1 sent\n");
        return EXIT_SUCCESS;
    }

    printf("[parent] waiting for SIGUSR1 from child %lu\n", (unsigned long)pid);
    while (!signal_received) {
        sigsuspend(&old_mask);
    }

    printf("[parent] received SIGUSR1, handling it now\n");
    waitpid(pid, &status, 0);

    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ?
           EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
