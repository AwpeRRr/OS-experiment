#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "[Child] 缺少管道读端文件描述符。\n");
        exit(1);
    }

    int read_fd = atoi(argv[1]);
    char read_msg[256];

    printf("[Child] 正在从匿名管道读取数据...\n");

    read(read_fd, read_msg, sizeof(read_msg));

    printf("[Child] 读取到的数据: %s\n", read_msg);

    close(read_fd);

    return 0;
}
