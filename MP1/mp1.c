#define LINUX

#include "mp1.h"    // mp1 header file

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
	ssize_t byte_read = 0;
	size_t len = 0;
	char *kbuf;
    proc_struct *ps;
	struct status_buf *sbuf;

	// check the access of the buffer
    if (!access_ok(buffer, size)) {
        printk(KERN_ERR "User Buffer is NOT WRITABLE\n");
		return -EINVAL;
	}

	// Check if the user has already read part of the buffer
	if (*offl || file->private_data) {
		sbuf = file->private_data;
		len = sbuf->size;
		kbuf = sbuf->buf;

        // check if the user has already reach EOF
		if (*offl >= len) {
		    return 0;

	    // copy the remain buf to user space
        } else {
			byte_read = (len- *offl < size) ? (len - *offl) : size;
            cpy_kernel_byte = copy_to_user(buffer, kbuf + *offl, byte_read);
            if (cpy_kernel_byte != 0) {
                printk(KERN_ERR "copy_to_user failed\n");
            }
			byte_read -= cpy_kernel_byte;   // update byte_read
			*offl += byte_read;             // update file offset
		}
	    return byte_read;
	}

    // allocate memory according to list size
	spin_lock_irqsave(&lock, flag);         // enter critical section, save flags
	list_for_each_entry(ps, &proc_list, list) ++len;
	spin_unlock_irqrestore(&lock, flag);    // exit critical section, restore flags

	sbuf = kmalloc(sizeof(struct status_buf) + len * MAX_STR_LEN, GFP_KERNEL);
	if (sbuf == NULL) {
        printk(KERN_ERR "Fail to allocate memory in kernel\n");
		return -ENOMEM;
	}

	len *= MAX_STR_LEN;
	kbuf = sbuf->buf;
	file->private_data = sbuf;
	
    spin_lock_irqsave(&lock, flag);         // enter critical section, save flags
	list_for_each_entry(ps, &proc_list, list) {
		byte_read += scnprintf(kbuf + byte_read, len - byte_read, "%d: %lu\n", ps->pid, ps->cpu_time);
		if (byte_read >= len) break;        // break if user read size or more byte
	}
	spin_unlock_irqrestore(&lock, flag);    // exit critical section, restore flags

	sbuf->size = byte_read;                 // store byte_read in status buf struct
	byte_read = (byte_read < size) ? byte_read : size;
    cpy_kernel_byte = copy_to_user(buffer, kbuf, byte_read);
    if (cpy_kernel_byte != 0) {
        printk(KERN_ERR "copy_to_user failed\n");
    }
	byte_read -= cpy_kernel_byte;           // update byte_read
	*offl += byte_read;                     // update file offset

	return byte_read;
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
    char *kbuf;
    proc_struct *ps;
    
    // check the access of the buffer
    if (!access_ok(buffer, size)) {
        printk(KERN_ERR "Buffer is NOT READABLE\n");
        return -EINVAL;
    }

    kbuf = (char *)kmalloc(size+ 1, GFP_KERNEL);
    if (kbuf == NULL) {
        printk(KERN_ERR "Fail to allocate memory in kernel\n");
        return -ENOMEM;
    }

    cpy_usr_byte = copy_from_user(kbuf, buffer, size);
    if (cpy_usr_byte != 0) {
        printk(KERN_ERR "copy_from_user fail\n");
    }

    ps = (proc_struct *)kmalloc(sizeof(proc_struct), GFP_KERNEL);

    kbuf[size] = '\0';                      // null terminate kbuf
    sscanf(kbuf, "%u", &ps->pid);           // read buf and write to ps->pid
    ps->cpu_time = 0;                       // init ps->cpu_time to 0

    spin_lock_irqsave(&lock, flag);         // enter critical section, save flags
    list_add_tail(&(ps->list), &proc_list); // add list to proc_list
    spin_unlock_irqrestore(&lock, flag);    // exit critical section, restore flags

    kfree(kbuf);

    return size;
}

/**
 * called when a process closes the file
 *
 * @param *inode ptr to inode
 * @param *file ptr to file
 * @return int 0-suceess, other-fail
 */
static int proc_release(struct inode *inode, struct file *file) {
    kfree(file->private_data);
    return 0;
}

/**
 * invoke callback once after the timer elapses
 *
 * @param   *timer  ptr to timer
 * @return  void
 */
static void callback(struct timer_list *timer) {
    queue_work(wq, work);
    mod_timer(timer, jiffies + msecs_to_jiffies(TIME_INTERVAL));
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

    spin_lock_irqsave(&lock, flag);         // enter critical section, save flags
    list_for_each_entry_safe(pos, n, &proc_list, list) {
        if (get_cpu_use(pos->pid, &cpu_time) == 0) {
			pos->cpu_time = cpu_time;       // update cpu_time
		} else {
			list_del(&pos->list);
			kfree(pos);
		}
    }
    spin_unlock_irqrestore(&lock, flag);    // exit critical section, restore flags
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
    
    kfree(work);                            // free work

    printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);