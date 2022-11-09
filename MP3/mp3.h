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

#include <linux/module.h>   /* module_init & module_exit                            */
#include <linux/kernel.h>
#include <linux/proc_fs.h>  /* proc_ops & proc_mkdir & proc_create & rm_proc_entry  */
#include <linux/slab.h>     /* kmalloc & kfree                                      */
#include <linux/vmalloc.h>  /* vmalloc                                              */
#include <linux/mm.h>       /* vmalloc_to_pfn & remap_pfn_range                     */
#include <linux/cdev.h>     /* struct cdev & cdev_init & cdev_add & cdev_del        */
#include "mp3_given.h"      /* mp3_given header file                                */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_ID");
MODULE_DESCRIPTION("CS-423 MP3");

#define DEBUG
#define FILENAME            "status"            /* proc filename            */
#define DIRECTORY           "mp3"               /* proc dir name            */
#define DEV_MAJOR_NUM       423                 /* character device major # */
#define DEV_MINOR_NUM       0                   /* character device minor # */
#define DEVICE_NAME         "cdev"              /* character device name    */
#define SAMPLE_FREQ         20                  /* sample frequency         */
#define DELAY_MS            1000/SAMPLE_FREQ    /* work delay in ms         */
#define NUM_PAGE            128                 /* # of page                */
#define MAX_PROF_BUF_SIZE   NUM_PAGE*4*1024     /* max profiler buf size    */
#define REGISTRATION        'R'                 /* registration             */
#define DE_REGISTRATION     'U'                 /* de-registration          */
#define PID_OFFSET          2                   /* buf offset for PID       */

static LIST_HEAD(work_proc_struct_list);                /* define work process struct list head         */
static DEFINE_SPINLOCK(lock);                           /* define spinlock for work process struct list */
static void update_data(struct work_struct *work);      /* define work function                         */
static DECLARE_DELAYED_WORK(prof_work, &update_data);   /* define delayed work                          */

/* define work process struct */
typedef struct work_proc_struct {
    pid_t pid;                          /* process ID       */
    struct task_struct *linux_task;     /* linux task       */
    struct list_head list;              /* list head        */
    unsigned long utilization;          /* utilization      */
    unsigned long maj_page_flt;         /* major page fault */
    unsigned long min_page_flt;         /* minor page fault */
} work_proc_struct_t;

static struct proc_dir_entry *proc_dir, *proc_entry;
static ssize_t proc_read(struct file *file, char __user *buffer, size_t size, loff_t *offset);
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t size, loff_t *offset);
static const struct proc_ops proc_fops = {
        .proc_read = proc_read,
        .proc_write = proc_write,
};

static dev_t dev = MKDEV(DEV_MAJOR_NUM, DEV_MINOR_NUM);                 /* chrdev major & minor #   */
static struct cdev cdev;                                                /* character device struct  */
static int cdev_mmap(struct file *file, struct vm_area_struct *vma);    /* mmap callback function   */
static const struct file_operations cdev_fops = {
        .owner = THIS_MODULE,
        .mmap = cdev_mmap,
        .open = NULL,
        .release = NULL,
};

struct workqueue_struct *wq;            /* workqueue struct     */
static unsigned long delay_jiffies;     /* delay in jiffies     */
static unsigned long *prof_buf;         /* profiler buffer      */
static unsigned prof_buf_ptr = 0;       /* profiler buffer ptr  */

#endif