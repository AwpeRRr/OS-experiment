#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INODES 64
#define MAX_BLOCKS 256
#define BLOCK_SIZE 64
#define MAX_NAME 32
#define MAX_CHILDREN 24
#define DIRECT_BLOCKS 4
#define INDIRECT_ENTRIES (BLOCK_SIZE / (int)sizeof(int))
#define GROUP_SIZE 16
#define MAX_GROUPS ((MAX_BLOCKS + GROUP_SIZE - 1) / GROUP_SIZE)
#define MAX_FILE_SIZE ((DIRECT_BLOCKS + INDIRECT_ENTRIES) * BLOCK_SIZE)

typedef enum InodeType {
    INODE_FREE = 0,
    INODE_FILE = 1,
    INODE_DIR = 2
} InodeType;

typedef struct Inode {
    int used;
    InodeType type;
    char name[MAX_NAME];
    int parent;
    int mode;
    int owner;
    int group;
    int size;
    int child_count;
    int children[MAX_CHILDREN];
    int direct[DIRECT_BLOCKS];
    int indirect;
} Inode;

typedef struct FreeBlockGroup {
    int count;
    int blocks[GROUP_SIZE];
    int next;
} FreeBlockGroup;

static Inode inodes[MAX_INODES];
static unsigned char data_blocks[MAX_BLOCKS][BLOCK_SIZE];
static int index_blocks[MAX_BLOCKS][INDIRECT_ENTRIES];
static int free_inode_stack[MAX_INODES];
static int free_inode_top = 0;
static FreeBlockGroup free_groups[MAX_GROUPS];
static int current_group = 0;
static int current_dir = 0;

static void reset_inode_blocks(Inode *inode) {
    int i;

    for (i = 0; i < DIRECT_BLOCKS; i++) {
        inode->direct[i] = -1;
    }
    inode->indirect = -1;
}

static void init_index_block(int block_no) {
    int i;

    for (i = 0; i < INDIRECT_ENTRIES; i++) {
        index_blocks[block_no][i] = -1;
    }
}

static void init_free_block_groups(void) {
    int i;
    int block_no;

    for (i = 0; i < MAX_GROUPS; i++) {
        free_groups[i].count = 0;
        free_groups[i].next = (i + 1 < MAX_GROUPS) ? i + 1 : -1;
    }

    for (block_no = 1; block_no < MAX_BLOCKS; block_no++) {
        int group = (block_no - 1) / GROUP_SIZE;
        int pos = free_groups[group].count;

        if (pos < GROUP_SIZE) {
            free_groups[group].blocks[pos] = block_no;
            free_groups[group].count++;
        }
    }

    current_group = 0;
}

static int allocate_block(void) {
    int block_no;

    while (current_group != -1 && free_groups[current_group].count == 0) {
        current_group = free_groups[current_group].next;
    }

    if (current_group == -1) {
        return -1;
    }

    free_groups[current_group].count--;
    block_no = free_groups[current_group].blocks[free_groups[current_group].count];
    memset(data_blocks[block_no], 0, BLOCK_SIZE);
    init_index_block(block_no);

    return block_no;
}

static void free_block(int block_no) {
    int group;

    if (block_no <= 0 || block_no >= MAX_BLOCKS) {
        return;
    }

    if (current_group == -1) {
        current_group = 0;
    }

    group = current_group;
    while (group != -1 && free_groups[group].count >= GROUP_SIZE) {
        group = free_groups[group].next;
    }

    if (group == -1) {
        for (group = 0; group < MAX_GROUPS; group++) {
            if (free_groups[group].count < GROUP_SIZE) {
                break;
            }
        }
    }

    if (group >= 0 && group < MAX_GROUPS) {
        free_groups[group].blocks[free_groups[group].count] = block_no;
        free_groups[group].count++;
        if (current_group == -1 || group < current_group) {
            current_group = group;
        }
    }
}

static void init_file_system(void) {
    int i;

    memset(inodes, 0, sizeof(inodes));
    memset(data_blocks, 0, sizeof(data_blocks));

    free_inode_top = 0;
    for (i = MAX_INODES - 1; i >= 1; i--) {
        free_inode_stack[free_inode_top] = i;
        free_inode_top++;
    }

    init_free_block_groups();

    inodes[0].used = 1;
    inodes[0].type = INODE_DIR;
    strcpy(inodes[0].name, "/");
    inodes[0].parent = 0;
    inodes[0].mode = 0755;
    inodes[0].owner = 0;
    inodes[0].group = 0;
    reset_inode_blocks(&inodes[0]);
    current_dir = 0;
}

static int valid_name(const char *name) {
    if (name == NULL || name[0] == '\0' || strlen(name) >= MAX_NAME) {
        return 0;
    }

    return strchr(name, '/') == NULL;
}

static int allocate_inode(InodeType type, const char *name, int parent) {
    int inode_no;

    if (free_inode_top <= 0) {
        return -1;
    }

    inode_no = free_inode_stack[--free_inode_top];
    memset(&inodes[inode_no], 0, sizeof(Inode));

    inodes[inode_no].used = 1;
    inodes[inode_no].type = type;
    strncpy(inodes[inode_no].name, name, MAX_NAME - 1);
    inodes[inode_no].parent = parent;
    inodes[inode_no].mode = type == INODE_DIR ? 0755 : 0644;
    inodes[inode_no].owner = 0;
    inodes[inode_no].group = 0;
    reset_inode_blocks(&inodes[inode_no]);

    return inode_no;
}

static int find_child(int dir, const char *name) {
    int i;

    if (!inodes[dir].used || inodes[dir].type != INODE_DIR) {
        return -1;
    }

    for (i = 0; i < inodes[dir].child_count; i++) {
        int child = inodes[dir].children[i];

        if (inodes[child].used && strcmp(inodes[child].name, name) == 0) {
            return child;
        }
    }

    return -1;
}

static int add_child(int dir, int child) {
    if (inodes[dir].child_count >= MAX_CHILDREN) {
        return 0;
    }

    inodes[dir].children[inodes[dir].child_count] = child;
    inodes[dir].child_count++;
    return 1;
}

static void remove_child(int dir, int child) {
    int i;

    for (i = 0; i < inodes[dir].child_count; i++) {
        if (inodes[dir].children[i] == child) {
            int j;

            for (j = i; j + 1 < inodes[dir].child_count; j++) {
                inodes[dir].children[j] = inodes[dir].children[j + 1];
            }
            inodes[dir].child_count--;
            return;
        }
    }
}

static int get_data_block(const Inode *inode, int logical_no) {
    if (logical_no < DIRECT_BLOCKS) {
        return inode->direct[logical_no];
    }

    if (inode->indirect == -1) {
        return -1;
    }

    logical_no -= DIRECT_BLOCKS;
    if (logical_no < 0 || logical_no >= INDIRECT_ENTRIES) {
        return -1;
    }

    return index_blocks[inode->indirect][logical_no];
}

static void release_file_blocks(int inode_no) {
    Inode *inode = &inodes[inode_no];
    int i;

    for (i = 0; i < DIRECT_BLOCKS; i++) {
        if (inode->direct[i] != -1) {
            free_block(inode->direct[i]);
            inode->direct[i] = -1;
        }
    }

    if (inode->indirect != -1) {
        for (i = 0; i < INDIRECT_ENTRIES; i++) {
            int block_no = index_blocks[inode->indirect][i];

            if (block_no != -1) {
                free_block(block_no);
                index_blocks[inode->indirect][i] = -1;
            }
        }

        free_block(inode->indirect);
        inode->indirect = -1;
    }

    inode->size = 0;
}

static int write_file_content(int inode_no, const char *content) {
    Inode *inode = &inodes[inode_no];
    int len = (int)strlen(content);
    int needed_blocks;
    int logical;
    int offset = 0;

    if (len > MAX_FILE_SIZE) {
        printf("error: file is too large, max size is %d bytes\n", MAX_FILE_SIZE);
        return 0;
    }

    release_file_blocks(inode_no);
    inode->size = len;
    needed_blocks = (len + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for (logical = 0; logical < needed_blocks; logical++) {
        int block_no = allocate_block();
        int copy_size = len - offset;

        if (block_no == -1) {
            printf("error: no free data block\n");
            release_file_blocks(inode_no);
            return 0;
        }

        if (copy_size > BLOCK_SIZE) {
            copy_size = BLOCK_SIZE;
        }

        memcpy(data_blocks[block_no], content + offset, (size_t)copy_size);

        if (logical < DIRECT_BLOCKS) {
            inode->direct[logical] = block_no;
        } else {
            int index_pos = logical - DIRECT_BLOCKS;

            if (inode->indirect == -1) {
                inode->indirect = allocate_block();
                if (inode->indirect == -1) {
                    printf("error: no free block for indirect index\n");
                    free_block(block_no);
                    release_file_blocks(inode_no);
                    return 0;
                }
            }
            index_blocks[inode->indirect][index_pos] = block_no;
        }

        offset += copy_size;
    }

    return 1;
}

static void free_inode_tree(int inode_no) {
    int i;

    if (inode_no <= 0 || inode_no >= MAX_INODES || !inodes[inode_no].used) {
        return;
    }

    if (inodes[inode_no].type == INODE_FILE) {
        release_file_blocks(inode_no);
    } else {
        for (i = inodes[inode_no].child_count - 1; i >= 0; i--) {
            free_inode_tree(inodes[inode_no].children[i]);
        }
    }

    memset(&inodes[inode_no], 0, sizeof(Inode));
    free_inode_stack[free_inode_top] = inode_no;
    free_inode_top++;
}

static void read_file_content(int inode_no, char *out, int out_size) {
    Inode *inode = &inodes[inode_no];
    int remaining = inode->size;
    int logical = 0;
    int out_pos = 0;

    while (remaining > 0 && out_pos + 1 < out_size) {
        int block_no = get_data_block(inode, logical);
        int copy_size = remaining;

        if (copy_size > BLOCK_SIZE) {
            copy_size = BLOCK_SIZE;
        }
        if (copy_size > out_size - out_pos - 1) {
            copy_size = out_size - out_pos - 1;
        }
        if (block_no == -1) {
            break;
        }

        memcpy(out + out_pos, data_blocks[block_no], (size_t)copy_size);
        out_pos += copy_size;
        remaining -= copy_size;
        logical++;
    }

    out[out_pos] = '\0';
}

static void print_file_content(int inode_no) {
    Inode *inode = &inodes[inode_no];
    int remaining = inode->size;
    int logical = 0;

    while (remaining > 0) {
        int block_no = get_data_block(inode, logical);
        int write_size = remaining > BLOCK_SIZE ? BLOCK_SIZE : remaining;

        if (block_no == -1) {
            break;
        }

        fwrite(data_blocks[block_no], 1, (size_t)write_size, stdout);
        remaining -= write_size;
        logical++;
    }

    putchar('\n');
}

static void cmd_mkdir(const char *name) {
    int inode_no;

    if (!valid_name(name)) {
        printf("error: invalid directory name\n");
        return;
    }
    if (find_child(current_dir, name) != -1) {
        printf("error: name already exists\n");
        return;
    }

    inode_no = allocate_inode(INODE_DIR, name, current_dir);
    if (inode_no == -1 || !add_child(current_dir, inode_no)) {
        printf("error: cannot create directory\n");
        if (inode_no != -1) {
            free_inode_tree(inode_no);
        }
        return;
    }

    printf("mkdir: %s\n", name);
}

static void cmd_mk(const char *name, const char *content) {
    int inode_no;

    if (!valid_name(name)) {
        printf("error: invalid file name\n");
        return;
    }
    if (find_child(current_dir, name) != -1) {
        printf("error: name already exists\n");
        return;
    }

    inode_no = allocate_inode(INODE_FILE, name, current_dir);
    if (inode_no == -1 || !add_child(current_dir, inode_no)) {
        printf("error: cannot create file\n");
        if (inode_no != -1) {
            free_inode_tree(inode_no);
        }
        return;
    }

    if (!write_file_content(inode_no, content == NULL ? "" : content)) {
        remove_child(current_dir, inode_no);
        free_inode_tree(inode_no);
        return;
    }

    printf("mk: %s (%d bytes)\n", name, inodes[inode_no].size);
}

static void cmd_ls(void) {
    int i;

    printf("type  mode owner group size name\n");
    for (i = 0; i < inodes[current_dir].child_count; i++) {
        int child = inodes[current_dir].children[i];
        Inode *node = &inodes[child];

        printf("%c     %03o  %-5d %-5d %-4d %s\n",
               node->type == INODE_DIR ? 'd' : '-',
               node->mode,
               node->owner,
               node->group,
               node->size,
               node->name);
    }
}

static void cmd_cd(const char *name) {
    int child;

    if (name == NULL || strcmp(name, "/") == 0) {
        current_dir = 0;
        return;
    }
    if (strcmp(name, "..") == 0) {
        current_dir = inodes[current_dir].parent;
        return;
    }

    child = find_child(current_dir, name);
    if (child == -1 || inodes[child].type != INODE_DIR) {
        printf("error: directory not found\n");
        return;
    }

    current_dir = child;
}

static void print_path_recursive(int inode_no) {
    if (inode_no == 0) {
        printf("/");
        return;
    }

    print_path_recursive(inodes[inode_no].parent);
    if (inodes[inode_no].parent != 0) {
        printf("/");
    }
    printf("%s", inodes[inode_no].name);
}

static void cmd_pwd(void) {
    print_path_recursive(current_dir);
    putchar('\n');
}

static void cmd_cat(const char *name) {
    int inode_no = find_child(current_dir, name);

    if (inode_no == -1 || inodes[inode_no].type != INODE_FILE) {
        printf("error: file not found\n");
        return;
    }

    print_file_content(inode_no);
}

static void cmd_rm(const char *name) {
    int inode_no = find_child(current_dir, name);

    if (inode_no == -1 || inodes[inode_no].type != INODE_FILE) {
        printf("error: file not found\n");
        return;
    }

    remove_child(current_dir, inode_no);
    free_inode_tree(inode_no);
    printf("rm: %s\n", name);
}

static void cmd_rmdir(const char *name) {
    int inode_no = find_child(current_dir, name);

    if (inode_no == -1 || inodes[inode_no].type != INODE_DIR) {
        printf("error: directory not found\n");
        return;
    }
    if (inodes[inode_no].child_count != 0) {
        printf("error: directory is not empty\n");
        return;
    }

    remove_child(current_dir, inode_no);
    free_inode_tree(inode_no);
    printf("rmdir: %s\n", name);
}

static void cmd_cp(const char *src, const char *dst) {
    int src_inode;
    char content[MAX_FILE_SIZE + 1];

    if (!valid_name(dst)) {
        printf("error: invalid target name\n");
        return;
    }
    if (find_child(current_dir, dst) != -1) {
        printf("error: target already exists\n");
        return;
    }

    src_inode = find_child(current_dir, src);
    if (src_inode == -1 || inodes[src_inode].type != INODE_FILE) {
        printf("error: source file not found\n");
        return;
    }

    read_file_content(src_inode, content, sizeof(content));
    cmd_mk(dst, content);
}

static void cmd_chmod(const char *mode_text, const char *name) {
    int inode_no = find_child(current_dir, name);

    if (inode_no == -1) {
        printf("error: target not found\n");
        return;
    }

    inodes[inode_no].mode = (int)strtol(mode_text, NULL, 8);
    printf("chmod: %s -> %03o\n", name, inodes[inode_no].mode);
}

static void cmd_chown(const char *owner_text, const char *name) {
    int inode_no = find_child(current_dir, name);

    if (inode_no == -1) {
        printf("error: target not found\n");
        return;
    }

    inodes[inode_no].owner = atoi(owner_text);
    printf("chown: %s -> %d\n", name, inodes[inode_no].owner);
}

static void cmd_chgrp(const char *group_text, const char *name) {
    int inode_no = find_child(current_dir, name);

    if (inode_no == -1) {
        printf("error: target not found\n");
        return;
    }

    inodes[inode_no].group = atoi(group_text);
    printf("chgrp: %s -> %d\n", name, inodes[inode_no].group);
}

static void cmd_chnam(const char *old_name, const char *new_name) {
    int inode_no = find_child(current_dir, old_name);

    if (inode_no == -1) {
        printf("error: target not found\n");
        return;
    }
    if (!valid_name(new_name) || find_child(current_dir, new_name) != -1) {
        printf("error: invalid or duplicate new name\n");
        return;
    }

    strncpy(inodes[inode_no].name, new_name, MAX_NAME - 1);
    inodes[inode_no].name[MAX_NAME - 1] = '\0';
    printf("chnam: %s -> %s\n", old_name, new_name);
}

static char *skip_spaces(char *text) {
    while (text != NULL && *text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }
    return text;
}

static char *next_token(char **cursor) {
    char *start;
    char *p;

    if (cursor == NULL || *cursor == NULL) {
        return NULL;
    }

    start = skip_spaces(*cursor);
    if (*start == '\0') {
        *cursor = start;
        return NULL;
    }

    p = start;
    while (*p != '\0' && !isspace((unsigned char)*p)) {
        p++;
    }

    if (*p != '\0') {
        *p = '\0';
        *cursor = p + 1;
    } else {
        *cursor = p;
    }

    return start;
}

static void show_help(void) {
    printf("commands:\n");
    printf("  mk <name> [text]       create a file\n");
    printf("  cp <src> <dst>         copy a file in current directory\n");
    printf("  mkdir <name>           create a directory\n");
    printf("  rmdir <name>           remove an empty directory\n");
    printf("  cd <name|..|/>         change current directory\n");
    printf("  ls                     list current directory\n");
    printf("  cat <name>             print file content\n");
    printf("  chmod <mode> <name>    change file mode, for example 755\n");
    printf("  chown <uid> <name>     change owner id\n");
    printf("  chgrp <gid> <name>     change group id\n");
    printf("  chnam <old> <new>      rename file or directory\n");
    printf("  rm <name>              remove a file\n");
    printf("  pwd                    print current directory\n");
    printf("  help                   show help\n");
    printf("  exit                   quit\n");
}

static void execute_line(char *line) {
    char *cursor = line;
    char *cmd = next_token(&cursor);
    char *a;
    char *b;

    if (cmd == NULL) {
        return;
    }

    if (strcmp(cmd, "mk") == 0) {
        a = next_token(&cursor);
        cmd_mk(a, skip_spaces(cursor));
    } else if (strcmp(cmd, "cp") == 0) {
        a = next_token(&cursor);
        b = next_token(&cursor);
        if (a != NULL && b != NULL) {
            cmd_cp(a, b);
        } else {
            printf("usage: cp <src> <dst>\n");
        }
    } else if (strcmp(cmd, "mkdir") == 0) {
        a = next_token(&cursor);
        cmd_mkdir(a);
    } else if (strcmp(cmd, "rmdir") == 0) {
        a = next_token(&cursor);
        cmd_rmdir(a);
    } else if (strcmp(cmd, "cd") == 0) {
        a = next_token(&cursor);
        cmd_cd(a);
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(cmd, "cat") == 0) {
        a = next_token(&cursor);
        cmd_cat(a);
    } else if (strcmp(cmd, "chmod") == 0) {
        a = next_token(&cursor);
        b = next_token(&cursor);
        if (a != NULL && b != NULL) {
            cmd_chmod(a, b);
        } else {
            printf("usage: chmod <mode> <name>\n");
        }
    } else if (strcmp(cmd, "chown") == 0) {
        a = next_token(&cursor);
        b = next_token(&cursor);
        if (a != NULL && b != NULL) {
            cmd_chown(a, b);
        } else {
            printf("usage: chown <uid> <name>\n");
        }
    } else if (strcmp(cmd, "chgrp") == 0) {
        a = next_token(&cursor);
        b = next_token(&cursor);
        if (a != NULL && b != NULL) {
            cmd_chgrp(a, b);
        } else {
            printf("usage: chgrp <gid> <name>\n");
        }
    } else if (strcmp(cmd, "chnam") == 0) {
        a = next_token(&cursor);
        b = next_token(&cursor);
        if (a != NULL && b != NULL) {
            cmd_chnam(a, b);
        } else {
            printf("usage: chnam <old> <new>\n");
        }
    } else if (strcmp(cmd, "rm") == 0) {
        a = next_token(&cursor);
        cmd_rm(a);
    } else if (strcmp(cmd, "pwd") == 0) {
        cmd_pwd();
    } else if (strcmp(cmd, "help") == 0) {
        show_help();
    } else {
        printf("error: unknown command, type help\n");
    }
}

int main(void) {
    char line[512];

    init_file_system();
    printf("Linux-like file system simulator\n");
    printf("It uses an inode stack, grouped free-block links, and mixed indexing.\n");
    show_help();

    while (1) {
        printf("fs:");
        cmd_pwd();
        printf("> ");

        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        line[strcspn(line, "\r\n")] = '\0';
        if (strcmp(skip_spaces(line), "exit") == 0) {
            break;
        }

        execute_line(line);
    }

    printf("file system simulator exited\n");
    return EXIT_SUCCESS;
}
