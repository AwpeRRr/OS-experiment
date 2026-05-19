// User-space process lister: iterate /proc and print PID + executable name
// Usage: ex9 [num]

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int is_digits(const char *s) {
    if (!s || !*s) return 0;
    for (; *s; ++s) if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

int main(int argc, char **argv) {
    long num = -1;
    if (argc > 1) {
        char *endptr;
        errno = 0;
        num = strtol(argv[1], &endptr, 10);
        if (errno || *endptr != '\0' || num < 0) {
            fprintf(stderr, "Usage: %s [num]\n", argv[0]);
            return 1;
        }
    }

    DIR *proc = opendir("/proc");
    if (!proc) {
        perror("opendir(/proc)");
        return 1;
    }

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(proc)) != NULL) {
        if (!is_digits(entry->d_name))
            continue;

        int pid = atoi(entry->d_name);

        char comm_path[PATH_MAX];
        snprintf(comm_path, sizeof(comm_path), "/proc/%s/comm", entry->d_name);

        char comm[256] = {0};
        FILE *f = fopen(comm_path, "r");
        if (f) {
            if (fgets(comm, sizeof(comm), f) != NULL) {
                size_t ln = strlen(comm);
                if (ln && comm[ln-1] == '\n') comm[ln-1] = '\0';
            }
            fclose(f);
        } else {
            // fallback: try reading /proc/<pid>/exe link
            char exe_path[PATH_MAX];
            char exe_real[PATH_MAX];
            snprintf(exe_path, sizeof(exe_path), "/proc/%s/exe", entry->d_name);
            ssize_t r = readlink(exe_path, exe_real, sizeof(exe_real) - 1);
            if (r != -1) {
                exe_real[r] = '\0';
                char *base = strrchr(exe_real, '/');
                if (base) strncpy(comm, base + 1, sizeof(comm)-1);
                else strncpy(comm, exe_real, sizeof(comm)-1);
            } else {
                strncpy(comm, "unknown", sizeof(comm)-1);
            }
        }

        printf("PID: %d, Executable Name: %s\n", pid, comm);
        count++;
        if (num >= 0 && count >= num) break;
    }

    closedir(proc);
    fprintf(stderr, "--- Printed %d process(es) ---\n", count);
    return 0;
}