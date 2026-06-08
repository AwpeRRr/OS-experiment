/*
 * Experiment 10: simple Linux kernel module.
 *
 * This file is for Linux kernel module programming.
 *
 * Example Makefile content:
 *     obj-m += standard_10.o
 *
 * Build on Ubuntu:
 *     make -C /lib/modules/$(uname -r)/build M=$PWD modules
 *
 * Run:
 *     sudo insmod standard_10.ko message=hello repeat=3
 *     dmesg | tail
 *     sudo rmmod standard_10
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

static char *message = "hello from OS experiment 10";
static int repeat = 1;

module_param(message, charp, 0644);
module_param(repeat, int, 0644);

static int __init standard_10_init(void)
{
    int i;

    if (repeat < 1) {
        repeat = 1;
    }

    pr_info("standard_10: module is loaded\n");

    for (i = 0; i < repeat; i++) {
        pr_info("standard_10: %s (%d/%d)\n", message, i + 1, repeat);
    }

    return 0;
}

static void __exit standard_10_exit(void)
{
    pr_info("standard_10: module is removed\n");
}

module_init(standard_10_init);
module_exit(standard_10_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Experiment");
MODULE_DESCRIPTION("A simple Linux kernel module for OS lab");
