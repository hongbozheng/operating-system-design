/**
 * FILENAME: mp3.c
 *
 * DESCRIPTION: mp3 kernel module file
 *              Virtual Memory Page Fault Profiler
 *
 * AUTHOR: Hongbo Zheng
 *
 * DATE: Saturday Nov 5th, 2022
 */

#define LINUX

#include "mp3.h"

/**
 * mmap callback
 * mapping the physical pages of the buffer to
 * the virtual address space of a requested user process
 *
 * @param file  struct file ptr
 * @param vma   struct vm_area_struct
 * @return int  0-success; negative error code - failed
 */
static int cdev_mmap(struct file *file, struct vm_area_struct *vma) {
    unsigned long i, pfn;
    unsigned long size = vma->vm_end - vma->vm_start;
    int ret;

	if (size > MAX_PROF_BUF_SIZE) {
		printk(KERN_ALERT "[KERN_ALERT]: MMAP size exceed MAX_PROF_BUF_SIZE\n");
		return -EINVAL;
	}
   
    for (i = 0; i < size; i+=PAGE_SIZE) {
        pfn = vmalloc_to_pfn((char *)prof_buf + i);
        if ((ret = remap_pfn_range(vma, vma->vm_start+i, pfn, PAGE_SIZE, vma->vm_page_prot)) < 0) {
            printk(KERN_ALERT "[KERN_ALERT]: Fail to remap kernel memory to userspace\n");
            return ret;
        }
    }

    return 0;
}

/**
 * work function to update data
 * work_proc->utilization
 * work_proc->maj_page_flt
 * work_proc->min_page_flt
 *
 * @param work
 * @return void
 */
void update_data(struct work_struct *work) {
    unsigned long flag;
    work_proc_struct_t *work_proc;
    unsigned long ttl_min_flt, ttl_maj_flt, ttl_util, utime, stime;
    ttl_min_flt = ttl_maj_flt = ttl_util = 0;

    spin_lock_irqsave(&lock, flag);
    list_for_each_entry(work_proc, &work_proc_struct_list, list){
        if (get_cpu_use(work_proc->pid, &work_proc->min_page_flt, &work_proc->maj_page_flt,
                        &utime, &stime) == 0) {
            work_proc->utilization = utime + stime;
            ttl_util += work_proc->utilization;         /* sum up utilization       */
            ttl_maj_flt += work_proc->maj_page_flt;     /* sum up major page fault  */
            ttl_min_flt += work_proc->min_page_flt;     /* sum up minor page fault  */
        }
    }
    spin_unlock_irqrestore(&lock, flag);

    prof_buf[prof_buf_ptr++] = jiffies;                 /* write cur time in jiffies to profiler buf    */
    prof_buf[prof_buf_ptr++] = ttl_min_flt;             /* write total minor page fault to profiler buf */
    prof_buf[prof_buf_ptr++] = ttl_maj_flt;             /* write total major page fault to profiler buf */
    prof_buf[prof_buf_ptr++] = ttl_util;                /* write total utilization to profiler buf      */

    if (prof_buf_ptr >= MAX_PROF_BUF_SIZE) {
        printk(KERN_ALERT "[KERN_ALERT]: PROFILE BUFFER FULL, RESET PROFILE BUFFER PTR\n");
        prof_buf_ptr = 0;
    }

    queue_delayed_work(wq, &prof_work, delay_jiffies);  /* enqueue delayed work with delay in jiffies   */
}

static ssize_t proc_read(struct file *file, char __user *buffer, size_t size, loff_t *offset) {
    unsigned long flag, cpy_kernel_byte;
    char *kbuf;
    ssize_t byte_read;
    work_proc_struct_t* work_proc;

   if(*offset > 0) return 0;        /* if file already read, return 0 */

   if (!access_ok(buffer, size)) {
       printk(KERN_ALERT "[KERN_ALERT]: User Buffer is NOT WRITABLE\n");
       return -EINVAL;
   }

    kbuf = (char *)kmalloc(size, GFP_KERNEL);
    if (kbuf == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to allocate kernel memory\n");
        return -ENOMEM;
    }

    spin_lock_irqsave(&lock, flag);
    list_for_each_entry(work_proc, &work_proc_struct_list, list){
        byte_read += sprintf(kbuf+byte_read, "%d\n", work_proc->pid);
    }
    spin_unlock_irqrestore(&lock, flag);

    kbuf[byte_read] = '\0';

    cpy_kernel_byte = copy_to_user(buffer, kbuf, byte_read);
    if (cpy_kernel_byte != 0) {
        printk(KERN_ALERT "[KEN_ALERT]: copy_from_user fail\n");
    }

    kfree(kbuf);

    *offset = byte_read;
    return byte_read;
}

/**
 * function that get the work_proc_struct_t *ptr
 * using the given pid
 *
 * @param pid process id
 * @return rms_task_struct_t *ptr
 *          if rms_task_struct found, return rms_task_struct_t *ptr
 *          if not found, return NULL
 */
work_proc_struct_t *__get_work_proc_by_pid(pid_t pid) {
    work_proc_struct_t *work_proc = NULL;
    list_for_each_entry(work_proc, &work_proc_struct_list, list) {
        if (work_proc->pid == pid) return work_proc;
    }
    return work_proc;
}

/**
 * function that register process
 *
 * @param   buf buf contains process info
 * @return  int 1-success; 0 or negative error code - failed
 */
int reg_proc(char *buf) {
    pid_t pid;
    unsigned long flag;

    sscanf(buf, "%d", &pid);
    printk(KERN_ALERT "[KERN_ALERT]: REGISTER PROCESS WITH PID %d\n", pid);

    /* check if the linux task with pid exists */
    struct task_struct *linux_task = find_task_by_pid(pid);
    if (linux_task == NULL) return 0;

    work_proc_struct_t* work_proc = (work_proc_struct_t *)kmalloc(sizeof(work_proc_struct_t), GFP_KERNEL);
    if (work_proc == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to allocate kernel memory\n");
        return -ENOMEM;
    }

    /* init strcut work_proc */
    work_proc->pid = pid;
    work_proc->linux_task = linux_task;
    work_proc->utilization = 0;
    work_proc->maj_page_flt = 0;
    work_proc->min_page_flt = 0;

    /* if it's the first work process, enqueue delayed work */
    if (list_empty(&work_proc_struct_list)) queue_delayed_work(wq, &prof_work, delay_jiffies);

    /* add new struct work_proc to list */
    spin_lock_irqsave(&lock, flag);
    list_add_tail(&work_proc->list, &work_proc_struct_list);
    spin_unlock_irqrestore(&lock, flag);

    return 1;
}

/**
 * function that de-register process
 *
 * @param *buf  buf contains process info
 * @return int  1-success; 0-failed
 */
int dereg_proc(char *buf) {
    pid_t pid;
    unsigned long flag;

    sscanf(buf, "%d", &pid);
    printk(KERN_ALERT "[KERN_ALERT]: DEREGISTER PROCESS WITH PID %d\n", pid);

    spin_lock_irqsave(&lock, flag);
    work_proc_struct_t *work_proc = __get_work_proc_by_pid(pid);
    if (work_proc == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Cannot find work_proc_struct in list\n");
        return 0;
    }
    list_del(&work_proc->list);
    spin_unlock_irqrestore(&lock, flag);

    /* if it's the last struct work_proc, wait and cancel delayed work */
    if (list_empty(&work_proc_struct_list)) cancel_delayed_work_sync(&prof_work);

    return 1;
}

/**
 * function to write /proc/mp3/status file
 *
 * @param *file     file to write
 * @param *buffer   user buffer
 * @param size      size of user buffer
 * @param *offset   offset in the file
 * @return ssize_t  number of byte written
 */
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t size, loff_t *offset) {
    unsigned long cpy_usr_byte;
    char *kbuf;

    if (!access_ok(buffer, size)) {
        printk(KERN_ALERT "[KERN_ALERT]: Buffer is NOT READABLE\n");
        return -EINVAL;
    }

    kbuf = (char *)kmalloc(size+1, GFP_KERNEL);
    if (kbuf == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to allocate kernel memory\n");
        return -ENOMEM;
    }

    cpy_usr_byte = copy_from_user(kbuf, buffer, size);
    if (cpy_usr_byte != 0) {
        printk(KERN_ALERT "[KERN_ALERT]: copy_from_user fail\n");
    }

    kbuf[size] = '\0';  /* terminate kbuf */

    switch (kbuf[0]) {
        case REGISTRATION:
            reg_proc(kbuf+PID_OFFSET);
            break;
        case DE_REGISTRATION:
            dereg_proc(kbuf+PID_OFFSET);
            break;
        default:
            printk(KERN_ALERT "[KERN_ALERT]: INVALID CMD FOR PROCESS\n");
            break;
    }

   kfree(kbuf);
   return size;
}

/**
 * called when mp1 module is loaded
 *
 * @param void
 * @return void
 */
int __init mp3_init(void) {
    int ret = 0;
    int i;
    #ifdef DEBUG
    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE LOADING\n");
    #endif

    /* create /proc/mp3 */
    if ((proc_dir = proc_mkdir(DIRECTORY, NULL)) == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to create /proc/" DIRECTORY "\n");
        return -ENOMEM;
    }

    /* create /proc/mp3/status */
    if ((proc_entry = proc_create(FILENAME, 0666, proc_dir, &proc_fops)) == NULL) {
        remove_proc_entry(DIRECTORY, NULL);
        printk(KERN_ALERT "[KERN_ALERT]: Fail to create /proc/" DIRECTORY "/" FILENAME "\n");
        return -ENOMEM;
    }

    /* register character device */
    if ((ret = register_chrdev_region(dev, 1, DEVICE_NAME)) < 0) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to register character device\n");
        goto rm_proc_entry;
    }

    /* init character device */
    cdev_init(&cdev, &cdev_fops);

    /* add character device */
    if ((ret = cdev_add(&cdev, dev, 1)) < 0) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to add character device\n");
        goto unreg_cdev;
    }

    /* create workqueue */
    if ((wq = create_workqueue("work_queue")) == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to create workqueue\n");
        ret = -ENOMEM;
        goto rm_cdev;
    }
    delay_jiffies = msecs_to_jiffies(DELAY_MS);

    /* allocate virtually contiguous memory */
    if ((prof_buf = vmalloc(MAX_PROF_BUF_SIZE)) == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to allocate virtually contiguous memory\n");
        ret = -ENOMEM;
        goto rm_wq;
    }
    memset(prof_buf, -1, MAX_PROF_BUF_SIZE);

    for (i = 0; i < MAX_PROF_BUF_SIZE; i+=PAGE_SIZE) {
        SetPageReserved(vmalloc_to_page((char *)prof_buf + i));
    }

    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE LOADED\n");
    return 0;

rm_wq:                                      /* if vmalloc failed                */
    destroy_workqueue(wq);
rm_cdev:                                    /* if create_workqueue failed       */
    cdev_del(&cdev);
unreg_cdev:                                 /* if cdev_add failed               */
    unregister_chrdev_region(dev, 1);
rm_proc_entry:                              /* if register_chrdev_region failed */
    remove_proc_entry(FILENAME, proc_dir);
    remove_proc_entry(DIRECTORY, NULL);
    return ret;
}

/**
 * called when mp3 module is unloaded
 *
 * @param void
 * @return void
 */
void __exit mp3_exit(void) {
    unsigned long flag;
    work_proc_struct_t *pos, *n;
    unsigned long i;
    #ifdef DEBUG
    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE UNLOADING\n");
    #endif

    remove_proc_entry(FILENAME, proc_dir);      /* remove /proc/mp3/status  */
    remove_proc_entry(DIRECTORY, NULL);         /* remove /proc/mp3         */

    cdev_del(&cdev);
    unregister_chrdev_region(dev, 1);

    if (delayed_work_pending(&prof_work))       /* check pending delayed work   */
        cancel_delayed_work_sync(&prof_work);   /* wait & cancel delayed work   */
    destroy_workqueue(wq);                      /* safely destroy workqueue     */

    spin_lock_irqsave(&lock, flag);
    list_for_each_entry_safe(pos, n, &work_proc_struct_list, list) {
        list_del(&pos->list);                   /* delete each element in list  */
        kfree(pos);                             /* free work_proc_struct        */
    }
    spin_unlock_irqrestore(&lock, flag);

    for (i = 0; i < MAX_PROF_BUF_SIZE; i+=PAGE_SIZE) {
        ClearPageReserved(vmalloc_to_page((char *)prof_buf + i));
    }

    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(mp3_init);
module_exit(mp3_exit);