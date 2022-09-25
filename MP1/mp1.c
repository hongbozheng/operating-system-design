#define LINUX

#include "mp1.h"

/**
 * function to read /proc file
 * 
 * @param *file     file to read
 * @param *buffer   user buffer
 * @param size      size of user buffer
 * @param *offl     offset in the file
 * @return ssize_t  number of byte copied
 */
static ssize_t proc_read(struct file *file, char __user *buffer, size_t size, loff_t *offl) {
    unsigned long flag, cpy_kernel_byte;
    unsigned long byte_cpy = 0;
    char *buf;
    proc_struct *ps;
    
    if (!access_ok(buffer, size)) {
        printk(KERN_ERR "User Buffer is NOT WRITABLE\n");
        return -EINVAL;
    }
    
    //Reference: https://www.kernel.org/doc/html/latest/core-api/memory-allocation.html
    buf = (char *)kmalloc(size, GFP_KERNEL);

    // enter critical section
    spin_lock_irqsave(&lock, flag);
    list_for_each_entry(ps, &proc_list, list) {
        byte_cpy += sprintf(buf + byte_cpy, "%u: %u\n", ps->pid, jiffies_to_msecs(clock_t_to_jiffies(ps->cpu_time)));
    }
    spin_unlock_irqrestore(&lock, flag);

    buf[byte_cpy] = '\0';

    cpy_kernel_byte = copy_to_user(buffer, buf, byte_cpy);
    if (cpy_kernel_byte != 0){
        printk(KERN_ERR "copy_to_user failed\n");
    }

    // Reference: https://www.kernel.org/doc/htmldocs/kernel-hacking/routines-kmalloc.html
    kfree(buf);

    return byte_cpy;
}

/**
 * function to write /proc file
 * 
 * @param *file     file to write
 * @param *buffer   user buffer
 * @param size      size of user buffer
 * @param *offl     offset in the file
 * @return size     number of byte written
 */
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t size, loff_t *offl) {
    unsigned long flag, cpy_usr_byte;
    char *buf;
    proc_struct *ps;
    
    if (!access_ok(buffer, size)) {
        printk(KERN_ERR "Buffer is NOT READABLE\n");
        return -EINVAL;
    }

    // initialize tmp->list
    ps = (proc_struct *)kmalloc(sizeof(proc_struct), GFP_KERNEL);
    INIT_LIST_HEAD(&(ps->list));

    // set tmp->pid
    buf = (char *)kmalloc(size+ 1, GFP_KERNEL);
    cpy_usr_byte = copy_from_user(buf, buffer, size);
    if (cpy_usr_byte != 0) {
        printk(KERN_ERR "copy_from_user fail\n");
    }

    buf[size] = '\0';
    sscanf(buf, "%u", &ps->pid);

    // initial tmp->cpu_time
    ps->cpu_time = 0;

    // add tmp to mp1_proc_list
    spin_lock_irqsave(&lock, flag);
    list_add(&(ps->list), &proc_list);
    spin_unlock_irqrestore(&lock, flag);

    kfree(buf);

    return size;
}

/**
 * invoke callback once after the timer elapses
 *
 * @param   *timer  ptr to timer
 * @return  void
 */
static void callback(struct timer_list *timer) {
    queue_work(wq, work);
}

/**
 * work function to update cpu time
 *
 * @param   *work ptr to work
 * @return  void
 */
static void update_cpu_time(struct work_struct *work) {
    unsigned long flag, cpu_time;
    proc_struct *pos, *n;

    // enter critical section
    spin_lock_irqsave(&lock, flag);
    list_for_each_entry_safe(pos, n, &proc_list, list) {
        if (get_cpu_use(pos->pid, &cpu_time) == 0) {
			pos->cpu_time = cpu_time;
		} else {
			list_del(&pos->list);
			kfree(pos);
		}
    }
    spin_unlock_irqrestore(&lock, flag);

    mod_timer(&timer, jiffies + msecs_to_jiffies(TIME_INTERVAL));
}

/**
 * called when mp1 module is loaded
 *
 * @param   void
 * @return  int 0-success, other-failed
 */
int __init mp1_init(void) {
    #ifdef DEBUG
    printk(KERN_ALERT "MP1 MODULE LOADING\n");
    #endif
    
    // create /proc/mp1 dir
    // Reference: https://tuxthink.blogspot.com/2012/01/creating-folder-under-proc-and-creating.html
    if ((proc_dir = proc_mkdir(DIRECTORY, NULL)) == NULL) {
        printk(KERN_ERR "Fail to create /proc/" DIRECTORY "\n");
        return -ENOMEM;
    }
    
    // create /proc/mp1/status
    if ((proc_entry = proc_create(FILENAME, 0666, proc_dir, &proc_fops)) == NULL) {
        remove_proc_entry(DIRECTORY, NULL);
        printk(KERN_ERR "Fail to create /proc/" DIRECTORY "/" FILENAME "\n");
        return -ENOMEM;
    }
    
    // init and start timer
    // Reference: https://embetronicx.com/tutorials/linux/device-drivers/using-kernel-timer-in-linux-device-driver/
    timer_setup(&timer, callback, 0);
    mod_timer(&timer, jiffies + msecs_to_jiffies(TIME_INTERVAL));
    
    // init workqueue
    // Reference: http://www.makelinux.net/ldd3/chp-7-sect-6.shtml
    if ((wq = create_workqueue("workqueue")) == NULL) {
        printk(KERN_ERR "Fail to create workqueue\n");
        return -ENOMEM;
    }

    // init work
    work = (struct work_struct *)kmalloc(sizeof(struct work_struct), GFP_KERNEL);
    INIT_WORK(work, update_cpu_time);

    // init spin_lock
    spin_lock_init(&lock);

    printk(KERN_ALERT "MP1 MODULE LOADED\n");
    return 0;   
}

/**
 * called when mp1 module is unloaded
 *
 * @param   void
 * @return  void
 */
void __exit mp1_exit(void) {
    proc_struct *pos, *n;

    #ifdef DEBUG
    printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
    #endif
    
    remove_proc_entry(FILENAME, proc_dir);  // remove /proc/mp1/status
    remove_proc_entry(DIRECTORY, NULL);     // remove /proc/mp1
    del_timer_sync(&timer);                 // free timer
    
    list_for_each_entry_safe(pos, n, &proc_list, list) {
        list_del(&pos->list);               // delete proc_list
        kfree(pos);                         // free proc_list
    }
    
    flush_workqueue(wq);                    // ensure that any scheduled work has run to completion
    destroy_workqueue(wq);                  // safely destroy a workqueue
    
    kfree(work);

    printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
