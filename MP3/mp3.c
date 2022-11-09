#define LINUX

#include "mp3.h"

static int cdev_mmap(struct file *file, struct vm_area_struct *vma) {
    unsigned long i, pfn;
    unsigned long size = vma->vm_end - vma->vm_start;
    int ret;

	if (size > MAX_PROF_BUF_SIZE) {
		printk(KERN_ALERT "[KERN_ALERT]: MMAP size exceed MAX_PROF_BUF_SIZE\n");
		return -EINVAL;
	}
   
    for (i = 0; i < size; i+=PAGE_SIZE) {
        pfn = vmalloc_to_pfn((void *)(((unsigned long)prof_buf) + i));
        // Reference: https://elixir.bootlin.com/linux/v5.15.63/source/mm/memory.c#L2452
        // Reference: https://www.kernel.org/doc/htmldocs/kernel-api/API-remap-pfn-range.html
        if ((ret = remap_pfn_range(vma, vma->vm_start+i, pfn, PAGE_SIZE, vma->vm_page_prot)) < 0) {
            printk(KERN_ALERT "[KERN_ALERT]: Fail to remap kernel memory to userspace\n");
            return ret;
        }
    }

    return 0;
}

void update_data(struct work_struct *work) {
    unsigned long flag;
    work_proc_struct_t *work_proc;
    unsigned long ttl_min_flt, ttl_maj_flt, ttl_util, utime, stime;
    ttl_min_flt = ttl_maj_flt = ttl_util = 0;

    spin_lock_irqsave(&lock, flag);
    list_for_each_entry(work_proc, &work_proc_struct_list, list_node){
        if (get_cpu_use(work_proc->pid, &work_proc->minor_page_fault, &work_proc->major_page_fault,
                        &utime, &stime) == 0) {
            work_proc->utilization = utime + stime;
            ttl_util += work_proc->utilization;
            ttl_maj_flt += work_proc->major_page_fault;
            ttl_min_flt += work_proc->minor_page_fault;
        }
    }
    spin_unlock_irqrestore(&lock, flag);

    prof_buf[prof_buf_ptr++] = jiffies;
    prof_buf[prof_buf_ptr++] = ttl_min_flt;
    prof_buf[prof_buf_ptr++] = ttl_maj_flt;
    prof_buf[prof_buf_ptr++] = ttl_util;

    if (prof_buf_ptr >= MAX_PROF_BUF_SIZE) {
        printk(KERN_ALERT "[KERN_ALERT]: PROFILE BUFFER FULL, RESET PROFILE BUFFER PTR\n");
        prof_buf_ptr = 0;
    }

    queue_delayed_work(wq, &prof_work, delay_jiffies);
}

static ssize_t proc_read(struct file *file, char __user *buffer, size_t size, loff_t *offset) {
    unsigned long flag, cpy_kernel_byte;
    char *kbuf;
    ssize_t byte_read;
    work_proc_struct_t* work_proc;

   if(*offset > 0) return 0;

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
    list_for_each_entry(work_proc, &work_proc_struct_list, list_node){
        byte_read += sprintf(kbuf+byte_read, "PID: %d\n", work_proc->pid);
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

work_proc_struct_t *__get_work_proc_by_pid(pid_t pid) {
    work_proc_struct_t *work_proc = NULL;
    list_for_each_entry(work_proc, &work_proc_struct_list, list_node) {
        if (work_proc->pid == pid) return work_proc;
    }
    return work_proc;
}

int reg_proc(char *buf) {
    pid_t pid;
    unsigned long flag;

    sscanf(buf, "%d", &pid);
    printk(KERN_ALERT "[KERN_ALERT]: REGISTER PROCESS WITH PID %d\n", pid);

    struct task_struct *linux_task = find_task_by_pid(pid);
    if (linux_task == NULL) return 0;

    work_proc_struct_t* work_proc = (work_proc_struct_t *)kmalloc(sizeof(work_proc_struct_t), GFP_KERNEL);
    if(work_proc == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to allocate kernel memory\n");
        return -ENOMEM;
    }

    work_proc->pid = pid;
    work_proc->linux_task = linux_task;
    work_proc->utilization = 0;
    work_proc->major_page_fault = 0;
    work_proc->minor_page_fault = 0;

    // Reference: https://man7.org/linux/man-pages/man3/list.3.html
    // Reference: https://www.kernel.org/doc/htmldocs/kernel-api/API-list-empty.html
    if (list_empty(&work_proc_struct_list))
        queue_delayed_work(wq, &prof_work, delay_jiffies);

    spin_lock_irqsave(&lock, flag);
    // Reference: https://www.kernel.org/doc/htmldocs/kernel-api/API-list-add-tail.html
    // Reference: https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch10s05.html
    list_add_tail(&work_proc->list_node, &work_proc_struct_list);
    spin_unlock_irqrestore(&lock, flag);

    return 1;
}

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
    list_del(&work_proc->list_node);
    spin_unlock_irqrestore(&lock, flag);

    if (list_empty(&work_proc_struct_list))
        cancel_delayed_work_sync(&prof_work);

    return 1;
}

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

    kbuf[size] = '\0';

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

// mp1_init - Called when module is loaded
int __init mp3_init(void) {
    int ret = 0;
    int i;
    #ifdef DEBUG
    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE LOADING\n");
    #endif

    if ((proc_dir = proc_mkdir(DIRECTORY, NULL)) == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to create /proc/" DIRECTORY "\n");
        return -ENOMEM;
    }

    if ((proc_entry = proc_create(FILENAME, 0666, proc_dir, &proc_fops)) == NULL) {
        remove_proc_entry(DIRECTORY, NULL);
        printk(KERN_ALERT "[KERN_ALERT]: Fail to create /proc/" DIRECTORY "/" FILENAME "\n");
        return -ENOMEM;
    }

    // Reference: https://www.kernel.org/doc/htmldocs/kernel-api/API-alloc-chrdev-region.html
    if ((ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME)) < 0) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to allocate character device\n");
        goto rm_proc_entry;
    }
    // Reference: https://www.kernel.org/doc/htmldocs/kernel-api/API-cdev-init.html
    cdev_init(&cdev, &cdev_fops);
    // Reference: https://www.kernel.org/doc/htmldocs/kernel-api/API-cdev-add.html
    if ((ret = cdev_add(&cdev, dev, 1)) < 0) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to add character device\n");
        goto unreg_cdev;
    }

    if ((wq = create_workqueue("work_queue")) == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to create workqueue\n");
        ret = -ENOMEM;
        goto rm_cdev;
    }
    // Reference: https://elixir.bootlin.com/linux/v5.15.63/source/include/linux/jiffies.h#L363
    delay_jiffies = msecs_to_jiffies(DELAY_MS);

    if ((prof_buf = vmalloc(MAX_PROF_BUF_SIZE)) == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to allocate virtually contiguous memory\n");
        ret = -ENOMEM;
        goto rm_wq;
    }
    memset(prof_buf, -1, MAX_PROF_BUF_SIZE);

    // Reference: https://linux-kernel-labs.github.io/refs/heads/master/labs/memory_mapping.html
    for (i = 0; i < MAX_PROF_BUF_SIZE; i+=PAGE_SIZE) {
        SetPageReserved(vmalloc_to_page((void *)(((unsigned long)prof_buf) + i)));
    }

    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE LOADED\n");
    return 0;

rm_wq:
    destroy_workqueue(wq);
rm_cdev:
    cdev_del(&cdev);
unreg_cdev:
    unregister_chrdev_region(dev, 1);
rm_proc_entry:
    remove_proc_entry(FILENAME, proc_dir);
    remove_proc_entry(FILENAME, NULL);
    return ret;
}

// mp3_exit - Called when module is unloaded
void __exit mp3_exit(void) {
    unsigned long flag;
    work_proc_struct_t *pos, *n;
    int i;
    #ifdef DEBUG
    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE UNLOADING\n");
    #endif

    remove_proc_entry(FILENAME, proc_dir);
    remove_proc_entry(DIRECTORY, NULL);

    // Reference: https://www.kernel.org/doc/htmldocs/kernel-api/API-cdev-del.html
    cdev_del(&cdev);
    unregister_chrdev_region(dev, 1);

    if (delayed_work_pending(&prof_work))
        // Reference: https://linuxtv.org/downloads/v4l-dvb-internals/device-drivers/API-cancel-delayed-work-sync.html
        // Reference: https://manpages.debian.org/testing/linux-manual-4.8/cancel_delayed_work.9
        // Reference: https://docs.huihoo.com/doxygen/linux/kernel/3.7/workqueue_8c.html
        cancel_delayed_work_sync(&prof_work);
    destroy_workqueue(wq);

    spin_lock_irqsave(&lock, flag);
    list_for_each_entry_safe(pos, n, &work_proc_struct_list, list_node) {
        list_del(&pos->list_node);
        kfree(pos);
    }
    spin_unlock_irqrestore(&lock, flag);

    // Reference: https://linux-kernel-labs.github.io/refs/heads/master/labs/memory_mapping.html
    for (i = 0; i < MAX_PROF_BUF_SIZE; i+=PAGE_SIZE) {
        ClearPageReserved(vmalloc_to_page((void *)(((unsigned long)prof_buf) + i)));
    }

    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(mp3_init);
module_exit(mp3_exit);