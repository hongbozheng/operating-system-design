#ifndef __MP1_H__
#define __MP1_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>     // lib for spin_lock_irqsave
#include <linux/slab.h>         // lib for kmalloc & kfree
#include <linux/proc_fs.h>      // lib for struct proc_dir_entry
#include <linux/list.h>         // lib for LIST_HEAD
#include <linux/timer.h>        // lib for timer
#include <linux/workqueue.h>    // lib for workqueue_struct
#include "mp1_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG           1           // debug flag
#define DIRECTORY       "mp1"       // directory name
#define FILENAME        "status"    // filename
#define TIME_INTERVAL   5000        // time interval

LIST_HEAD(proc_list);               // init proc_list

// Reference: https://www.gnu.org/software/libc/manual/html_node/Data-Structures.html
typedef struct {
    struct list_head list;
    pid_t pid;
    unsigned long cpu_time;
} proc_struct;

static spinlock_t lock; // Reference: https://docs.oracle.com/cd/E26502_01/html/E35303/ggecq.html

// Reference: https://tuxthink.blogspot.com/2012/01/creating-folder-under-proc-and-creating.html
// struct hold info about the /proc file
static struct proc_dir_entry *proc_dir, *proc_entry;

// Reference: https://tldp.org/LDP/lkmpg/2.4/html/c577.htm
// Reference: https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch03s03.html
static ssize_t proc_read(struct file *file, char __user *buf, size_t size, loff_t *offl);
static ssize_t proc_write(struct file *file, const char __user *buf, size_t size, loff_t *offl);
static const struct proc_ops proc_fops = {
    .proc_read  = proc_read,
    .proc_write = proc_write
};

static struct timer_list timer;
// Reference: https://docs.kernel.org/core-api/workqueue.html
static struct workqueue_struct *wq;
static struct work_struct *work;

// function signature
static ssize_t proc_read(struct file *file, char __user *buffer, size_t size, loff_t *offl);
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t size, loff_t *offl);
static void callback(struct timer_list *timer);
static void update_cpu_time(struct work_struct *work);
int __init mp1_init(void);
void __exit mp1_exit(void);

#endif
