/*
 * Experiment 9: show simple process control block information.
 *
 * Compile on Ubuntu:
 *     gcc standard_9.c -o standard_9
 *
 * Run:
 *     ./standard_9
 *     ./standard_9 5
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

int is_number(const char *text)
{
    int i;

    for (i = 0; text[i] != '\0'; i++) {
        if (!isdigit((unsigned char)text[i])) {
            return 0;
        }
    }

    return text[0] != '\0';
}

void copy_value(char *dest, int dest_size, const char *line)
{
    const char *p = strchr(line, ':');
    int i = 0;

    if (p == NULL) {
        dest[0] = '\0';
        return;
    }

    p++;
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    while (*p != '\0' && *p != '\n' && i < dest_size - 1) {
        dest[i] = *p;
        i++;
        p++;
    }

    dest[i] = '\0';
}

void print_process_info(const char *pid_text)
{
    char path[128];
    char line[256];
    char name[64] = "";
    char state[64] = "";
    char ppid[64] = "";
    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%s/status", pid_text);

    fp = fopen(path, "r");
    if (fp == NULL) {
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, "Name:", 5) == 0) {
            copy_value(name, sizeof(name), line);
        } else if (strncmp(line, "State:", 6) == 0) {
            copy_value(state, sizeof(state), line);
        } else if (strncmp(line, "PPid:", 5) == 0) {
            copy_value(ppid, sizeof(ppid), line);
        }
    }

    fclose(fp);

    printf("PID=%s  PPID=%s  STATE=%s  NAME=%s\n",
           pid_text, ppid, state, name);
}

int main(int argc, char *argv[])
{
    DIR *dir;
    struct dirent *entry;
    int limit = 10;
    int count = 0;

    if (argc == 2) {
        limit = atoi(argv[1]);
    }

    dir = opendir("/proc");
    if (dir == NULL) {
        perror("opendir failed");
        return 1;
    }

    printf("Simple PCB information from /proc:\n");

    while ((entry = readdir(dir)) != NULL) {
        if (is_number(entry->d_name)) {
            print_process_info(entry->d_name);
            count++;

            if (limit > 0 && count >= limit) {
                break;
            }
        }
    }

    closedir(dir);
    printf("Printed %d process records.\n", count);

    return 0;
}
