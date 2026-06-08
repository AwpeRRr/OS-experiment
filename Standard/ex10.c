#ifdef __KERNEL__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

static char *message = "openEuler kernel module experiment";
static int repeat = 1;

module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "message printed by the module");
module_param(repeat, int, 0644);
MODULE_PARM_DESC(repeat, "number of messages printed when the module loads");

static int __init openeuler_module_init(void) {
    int i;

    if (repeat < 1) {
        repeat = 1;
    }

    pr_info("ex10: openEuler module loaded\n");
    for (i = 0; i < repeat; i++) {
        pr_info("ex10: %s (%d/%d)\n", message, i + 1, repeat);
    }

    return 0;
}

static void __exit openeuler_module_exit(void) {
    pr_info("ex10: openEuler module removed\n");
}

module_init(openeuler_module_init);
module_exit(openeuler_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Experiment");
MODULE_DESCRIPTION("openEuler kernel module programming experiment");

#else

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("openEuler operating system experiment\n");
    printf("Task 1: install openEuler and verify the login environment.\n");
    printf("Task 2: obtain, configure, compile, and install the openEuler kernel.\n");
    printf("Task 3: build ex10.c as a kernel module in the openEuler kernel tree.\n\n");

    printf("Example module build steps on openEuler:\n");
    printf("  1. Save this source file as ex10.c in a module directory.\n");
    printf("  2. Create a Makefile with: obj-m += ex10.o\n");
    printf("  3. Run: make -C /lib/modules/$(uname -r)/build M=$PWD modules\n");
    printf("  4. Load: sudo insmod ex10.ko message=hello repeat=3\n");
    printf("  5. Check logs: dmesg | tail\n");
    printf("  6. Remove: sudo rmmod ex10\n\n");

    printf("This user-mode output is only a guide. In the real openEuler task, "
           "the code above the __KERNEL__ branch is the kernel module body.\n");
    return EXIT_SUCCESS;
}

#endif
