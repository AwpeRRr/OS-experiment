#ifdef __KERNEL__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched/signal.h>

static int num = -1;
module_param(num, int, 0644);
MODULE_PARM_DESC(num, "number of process control blocks to print; negative means all");

static int __init pcb_info_init(void) {
    struct task_struct *task;
    int count = 0;

    pr_info("ex9: process control block list begins, num=%d\n", num);

    for_each_process(task) {
        if (num >= 0 && count >= num) {
            break;
        }

        pr_info("ex9: PID=%d COMM=%s\n", task->pid, task->comm);
        count++;
    }

    pr_info("ex9: printed %d process control block(s)\n", count);
    return 0;
}

static void __exit pcb_info_exit(void) {
    pr_info("ex9: module removed\n");
}

module_init(pcb_info_init);
module_exit(pcb_info_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Experiment");
MODULE_DESCRIPTION("Print process control block information in kernel mode");

#else

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int parse_num(int argc, char *argv[], long *num) {
    char *endptr;

    *num = -1;
    if (argc <= 1) {
        return 1;
    }

    errno = 0;
    *num = strtol(argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        return 0;
    }

    return 1;
}

#ifdef _WIN32
static int list_processes(long num) {
    HANDLE snapshot;
    PROCESSENTRY32 entry;
    long count = 0;

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "CreateToolhelp32Snapshot failed, error: %lu\n",
                (unsigned long)GetLastError());
        return EXIT_FAILURE;
    }

    memset(&entry, 0, sizeof(entry));
    entry.dwSize = sizeof(entry);

    if (!Process32First(snapshot, &entry)) {
        CloseHandle(snapshot);
        return EXIT_FAILURE;
    }

    do {
        if (num >= 0 && count >= num) {
            break;
        }

#ifdef UNICODE
        wprintf(L"PID=%lu COMM=%ls\n",
                (unsigned long)entry.th32ProcessID,
                entry.szExeFile);
#else
        printf("PID=%lu COMM=%s\n",
               (unsigned long)entry.th32ProcessID,
               entry.szExeFile);
#endif
        count++;
    } while (Process32Next(snapshot, &entry));

    CloseHandle(snapshot);
    printf("printed %ld process(es) in user-mode fallback\n", count);
    return EXIT_SUCCESS;
}
#else
static int is_digits(const char *text) {
    if (text == NULL || *text == '\0') {
        return 0;
    }

    while (*text != '\0') {
        if (!isdigit((unsigned char)*text)) {
            return 0;
        }
        text++;
    }

    return 1;
}

static int list_processes(long num) {
    DIR *proc_dir;
    struct dirent *entry;
    long count = 0;

    proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("opendir(/proc)");
        return EXIT_FAILURE;
    }

    while ((entry = readdir(proc_dir)) != NULL) {
        char comm_path[PATH_MAX];
        char comm[256];
        FILE *fp;

        if (!is_digits(entry->d_name)) {
            continue;
        }
        if (num >= 0 && count >= num) {
            break;
        }

        snprintf(comm_path, sizeof(comm_path), "/proc/%s/comm", entry->d_name);
        fp = fopen(comm_path, "r");
        if (fp == NULL) {
            continue;
        }

        if (fgets(comm, sizeof(comm), fp) != NULL) {
            comm[strcspn(comm, "\r\n")] = '\0';
            printf("PID=%s COMM=%s\n", entry->d_name, comm);
            count++;
        }

        fclose(fp);
    }

    closedir(proc_dir);
    printf("printed %ld process(es) in user-mode fallback\n", count);
    return EXIT_SUCCESS;
}
#endif

int main(int argc, char *argv[]) {
    long num;

    if (!parse_num(argc, argv, &num)) {
        fprintf(stderr, "usage: %s [num]\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("ex9 user-mode fallback: kernel module mode is used only when this "
           "file is built by the Linux kernel build system.\n");
    return list_processes(num);
}

#endif
