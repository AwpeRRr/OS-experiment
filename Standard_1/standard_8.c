/*
 * Experiment 8: simple file system simulator.
 *
 * Compile on Ubuntu:
 *     gcc standard_8.c -o standard_8
 *
 * Run:
 *     ./standard_8
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILES 10
#define MAX_NAME 32
#define MAX_CONTENT 128

struct FileNode {
    int used;
    char name[MAX_NAME];
    char content[MAX_CONTENT];
    int size;
};

struct FileNode file_table[MAX_FILES];

int find_file(const char *name)
{
    int i;

    for (i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

int find_free_slot(void)
{
    int i;

    for (i = 0; i < MAX_FILES; i++) {
        if (!file_table[i].used) {
            return i;
        }
    }

    return -1;
}

void list_files(void)
{
    int i;

    printf("File table:\n");
    printf("index  size  name\n");

    for (i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used) {
            printf("%5d  %4d  %s\n", i, file_table[i].size, file_table[i].name);
        }
    }
}

void create_file(char *name, char *text)
{
    int slot;

    if (name == NULL || text == NULL) {
        printf("usage: create <name> <text>\n");
        return;
    }

    if (find_file(name) >= 0) {
        printf("error: file already exists\n");
        return;
    }

    slot = find_free_slot();
    if (slot < 0) {
        printf("error: file table is full\n");
        return;
    }

    file_table[slot].used = 1;
    strncpy(file_table[slot].name, name, MAX_NAME - 1);
    file_table[slot].name[MAX_NAME - 1] = '\0';
    strncpy(file_table[slot].content, text, MAX_CONTENT - 1);
    file_table[slot].content[MAX_CONTENT - 1] = '\0';
    file_table[slot].size = strlen(file_table[slot].content);

    printf("create file: %s\n", file_table[slot].name);
}

void read_file(char *name)
{
    int pos;

    if (name == NULL) {
        printf("usage: read <name>\n");
        return;
    }

    pos = find_file(name);
    if (pos < 0) {
        printf("error: file not found\n");
        return;
    }

    printf("file name: %s\n", file_table[pos].name);
    printf("file size: %d\n", file_table[pos].size);
    printf("file data: %s\n", file_table[pos].content);
}

void delete_file(char *name)
{
    int pos;

    if (name == NULL) {
        printf("usage: delete <name>\n");
        return;
    }

    pos = find_file(name);
    if (pos < 0) {
        printf("error: file not found\n");
        return;
    }

    file_table[pos].used = 0;
    printf("delete file: %s\n", name);
}

void show_help(void)
{
    printf("commands:\n");
    printf("  create <name> <text>  create a file\n");
    printf("  read <name>           read a file\n");
    printf("  delete <name>         delete a file\n");
    printf("  ls                    list files\n");
    printf("  help                  show help\n");
    printf("  exit                  quit\n");
}

int main(void)
{
    char line[256];
    char *cmd;
    char *name;
    char *text;

    printf("Simple file system simulator starts.\n");
    show_help();

    while (1) {
        printf("fs> ");

        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        line[strcspn(line, "\n")] = '\0';

        cmd = strtok(line, " ");
        if (cmd == NULL) {
            continue;
        }

        if (strcmp(cmd, "create") == 0) {
            name = strtok(NULL, " ");
            text = strtok(NULL, "");
            create_file(name, text);
        } else if (strcmp(cmd, "read") == 0) {
            name = strtok(NULL, " ");
            read_file(name);
        } else if (strcmp(cmd, "delete") == 0) {
            name = strtok(NULL, " ");
            delete_file(name);
        } else if (strcmp(cmd, "ls") == 0) {
            list_files();
        } else if (strcmp(cmd, "help") == 0) {
            show_help();
        } else if (strcmp(cmd, "exit") == 0) {
            break;
        } else {
            printf("unknown command, type help\n");
        }
    }

    printf("Simple file system simulator ends.\n");
    return 0;
}
