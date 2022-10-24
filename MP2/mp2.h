/**
 * FILENAME: mp2.h
 *
 * DESCRIPTION: mp2 kernel module header file
 *
 * AUTHOR: Hongbo Zheng
 *
 * DATE: Sunday Oct 23th, 2022
 */

#ifndef __MP2_H__
#define __MP2_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>             // KMEM_CACHE
#include <linux/kthread.h>          // kthread_run & kthread_should_stop
#include <uapi/linux/sched/types.h> // struct sched_attr
#include "mp2_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP2");

#define DEBUG           1
#define FILENAME        "status"
#define DIRECTORY       "mp2"
#define REGISTRATION    'R'
#define YIELD           'Y'
#define DE_REGISTRATION 'D'
#define SLEEPING        1
#define READY           2
#define RUNNING         3
#define UTIL_BOUND      69300
#define MULTIPLIER      100000

LIST_HEAD(rms_task_struct_list);
DEFINE_MUTEX(task_list_mutex);
DEFINE_MUTEX(cur_task_mutex);

typedef struct rms_task_struct{
    struct task_struct *linux_task;
    struct timer_list wakeup_timer;
    struct list_head list;
    pid_t pid;
    unsigned long period;           // ms
    unsigned long computation;      // ms
    unsigned long deadline;         // jiffies
    unsigned int state;
} rms_task_struct;

static spinlock_t lock;
static struct proc_dir_entry *proc_dir, *proc_entry;
static ssize_t proc_read(struct file *file, char __user *buffer, size_t size, loff_t *loff);
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t size, loff_t *loff);
static const struct proc_ops proc_fops = {
        .proc_read    = proc_read,
        .proc_write   = proc_write
};
static struct task_struct *dispatch_thread;
static rms_task_struct *cur_task = NULL;
static struct kmem_cache *rms_task_struct_cache;

#endif