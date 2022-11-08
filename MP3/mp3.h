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
#include <linux/mutex.h>
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

LIST_HEAD(work_proc_struct_list);

typedef struct monitor_task_struct {
    struct task_struct *linux_task;
    struct list_head list_node;
    unsigned int pid;
    unsigned long utilization;
    unsigned long major_page_fault;
    unsigned long minor_page_fault;
} work_proc_struct_t;

static ssize_t proc_read(struct file *file, char __user *buffer, size_t size, loff_t *offset);
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t size, loff_t *data);

static const struct proc_ops mp3_file = {
        .proc_read = proc_read,
        .proc_write = proc_write,
};

#endif