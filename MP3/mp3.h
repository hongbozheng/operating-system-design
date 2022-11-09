/**
 * FILENAME: mp3.h
 *
 * DESCRIPTION: mp3 kernel module header file
 *
 * AUTHOR: Hongbo Zheng
 *
 * DATE: Saturday Nov 5th, 2022
 */

#ifndef __MP3_H__
#define __MP3_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/sched.h>
//#include <asm-generic/atomic-long.h>
#include <linux/page-flags.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include "mp3_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP3");

#define DEBUG
#define FILENAME "status"
#define DIRECTORY "mp3"
#define MAJOR_NUM 423
#define MINOR_NUM 0
#define DEVICE_NAME "cdev"
#define SAMPLE 20
#define DELAY_MS 1000/SAMPLE
#define NUM_PAGE 128
#define MAX_PROF_BUF_SIZE NUM_PAGE*4*1024
#define REGISTRATION 'R'
#define DE_REGISTRATION 'U'
#define PID_OFFSET 2

static LIST_HEAD(work_proc_struct_list);
static DEFINE_SPINLOCK(lock);
static void update_data(struct work_struct *work);
static DECLARE_DELAYED_WORK(prof_work, &update_data);

typedef struct work_proc_struct {
    struct task_struct *linux_task;
    struct list_head list_node;
    pid_t pid;
    unsigned long utilization;
    unsigned long major_page_fault;
    unsigned long minor_page_fault;
} work_proc_struct_t;

static struct proc_dir_entry *proc_dir, *proc_entry;
static ssize_t proc_read(struct file *file, char __user *buffer, size_t size, loff_t *offset);
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t size, loff_t *offset);
static const struct proc_ops proc_fops = {
        .proc_read = proc_read,
        .proc_write = proc_write,
};

static dev_t dev = MKDEV(MAJOR_NUM, MINOR_NUM);
// Reference: https://elixir.bootlin.com/linux/v5.15.63/source/include/linux/cdev.h#L14
static struct cdev cdev;
// Reference: https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch13s02.html
static int cdev_mmap(struct file *file, struct vm_area_struct *vma);
// Reference: https://elixir.bootlin.com/linux/latest/source/include/linux/fs.h#L2093
// Reference: https://tldp.org/LDP/lkmpg/2.4/html/c577.htm
// Reference: https://docs.huihoo.com/doxygen/linux/kernel/3.7/structfile__operations.html
// Reference: https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch03s03.html
static const struct file_operations cdev_fops = {
        .owner = THIS_MODULE,
        .mmap = cdev_mmap,
        .open = NULL,
        .release = NULL,
};

struct workqueue_struct *wq;
static unsigned long delay_jiffies;
static unsigned long *prof_buf;
static unsigned prof_buf_ptr = 0;

#endif