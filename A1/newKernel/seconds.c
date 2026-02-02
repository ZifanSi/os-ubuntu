/*
 * seconds.c
 *
 * Kernel module that communicates with /proc file system
 * Operating Systems SFWRENG 3SH3
 * 
 * Stanislav Serbezov
 * Add ur name here
 *
 * Output of lsb_release -a
 * No LSB modules are available.
 * Distributor ID:	Ubuntu
 * Description:	Ubuntu 25.10
 * Release:	25.10
 * Codename:	questing
 *
 * Output of uname -r
 * 6.17.0-12-generic
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define BUFFER_SIZE 128
#define PROC_NAME "seconds"

/* store jiffies when the module loads */
static unsigned long start_jiffies;

/* Function prototypes */
ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos);

static const struct proc_ops my_proc_ops = {
        .proc_read = proc_read,
};

/* This function is called when the module is loaded. */
int proc_init(void)
{
        start_jiffies = jiffies;

        /* creates the /proc/seconds entry */
        proc_create(PROC_NAME, 0, NULL, &my_proc_ops);

        printk(KERN_INFO "/proc/%s created\n", PROC_NAME);

        return 0;
}

/* This function is called when the module is removed. */
void proc_exit(void)
{
        /* removes the /proc/seconds entry */
        remove_proc_entry(PROC_NAME, NULL);

        printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

/*
 * This function is called each time /proc/seconds is read.
 */
ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
        int rv = 0;
        char buffer[BUFFER_SIZE];
        static int completed = 0;

        unsigned long elapsed_seconds;

        if (completed) {
                completed = 0;
                return 0;
        }

        completed = 1;

        elapsed_seconds = (jiffies - start_jiffies) / HZ;

        rv = sprintf(buffer, "%lu\n", elapsed_seconds);

        /* copy to user space */
        copy_to_user(usr_buf, buffer, rv);

        return rv;
}

/* Macros for registering module entry and exit points. */
module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Seconds Module");
MODULE_AUTHOR("Stan and Zifan");