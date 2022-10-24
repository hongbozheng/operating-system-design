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

#define DEBUG            1
#define FILENAME         "status"
#define DIRECTORY        "mp2"
#define REGISTRATION     'R'
#define YIELD            'Y'
#define DEREGISTRATION   'D'
#define SLEEPING         1
#define READY            2
#define RUNNING          3

LIST_HEAD(rms_task_struct_list);
DEFINE_MUTEX(struct_list_mutex);
DEFINE_MUTEX(cur_running_task_mutex);

typedef struct rms_task_struct{
    struct task_struct *linux_task;
    struct timer_list wakeup_timer;
    struct list_head list;

    // milliseconds
    pid_t pid;
    unsigned long period;
    unsigned long computation;
    unsigned int state;
    // jiffies
    unsigned long deadline;
} rms_task_struct;

static spinlock_t lock;
static struct proc_dir_entry *proc_dir, *proc_entry;
static struct task_struct *dispatch_thread;
static rms_task_struct *current_running_task = NULL;
static struct kmem_cache *mp2_task_struct_cache;

#endif