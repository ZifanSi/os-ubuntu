/**
 * simple.c
 *
 * A simple kernel module.
 *
 * To compile, run makefile by entering "make"
 *
 * Operating System Concepts - 10th Edition
 * Copyright John Wiley & Sons - 2018
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/sched.h>
#include <linux/version.h>

/* Print selected fields of the init_task (pid 0) PCB. */
static void print_init_PCB(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0)
    long st = init_task.__state;   /* newer kernels */
#else
    long st = init_task.state;     /* older kernels */
#endif

    printk(KERN_INFO "init_task pid:%d\n", init_task.pid);
    printk(KERN_INFO "init_task state:%ld\n", st);
    printk(KERN_INFO "init_task flags:%lu\n", init_task.flags);
    printk(KERN_INFO "init_task runtime priority:%u\n", init_task.rt_priority);
    printk(KERN_INFO "init_task process policy:%u\n", init_task.policy);
    printk(KERN_INFO "init_task tgid:%d\n", init_task.tgid);
}

/* This function is called when the module is loaded. */
static int simple_init(void)
{
    printk(KERN_INFO "Loading Module\n");
    print_init_PCB();
    return 0;
}

/* This function is called when the module is removed. */
static void simple_exit(void)
{
    printk(KERN_INFO "Removing Module\n");
}

/* Macros for registering module entry and exit points. */
module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module");
MODULE_AUTHOR("SGG");
