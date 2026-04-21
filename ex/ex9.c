#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/signal.h> // 需要此头文件来遍历进程

// 接收用户传递的参数 num
static int num = -1;
module_param(num, int, 0644);
MODULE_PARM_DESC(num, "Number of processes to print (default: all)");

static int __init process_info_init(void) {
    struct task_struct *task;
    int count = 0;

    printk(KERN_INFO "--- 进程控制块信息展示模块已加载 ---\n");

    // 遍历双向链表中的所有进程
    for_each_process(task) {
        if (num >= 0 && count >= num) {
            break; // 如果指定了num且大于等于0，达到数量则停止
        }
        // 打印 进程PID 和 进程的可执行文件名
        printk(KERN_INFO "PID: %d, Executable Name: %s\n", task->pid, task->comm);
        count++;
    }

    printk(KERN_INFO "--- 共打印了 %d 个进程信息 ---\n", count);
    return 0;
}

static void __exit process_info_exit(void) {
    printk(KERN_INFO "--- 进程控制块信息展示模块已卸载 ---\n");
}

module_init(process_info_init);
module_exit(process_info_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Student");
MODULE_DESCRIPTION("Display Process Control Block Info");