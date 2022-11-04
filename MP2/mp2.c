/**
 * FILENAME: mp2.c
 *
 * DESCRIPTION: mp2 kernel module file
 *
 * AUTHOR: Hongbo Zheng
 *
 * DATE: Sunday Oct 23th, 2022
 */

#define LINUX

#include "mp2.h"

/**
 * function that get the rms_task_struct_t *ptr
 * using the given pid
 *
 * @param pid process id
 * @return rms_task_struct_t *ptr
 */
rms_task_struct_t *__get_task_by_pid(pid_t pid) {
    rms_task_struct_t *task;
    list_for_each_entry(task, &rms_task_struct_list, list) {
        if (task->pid == pid) return task;  // found task, return task
    }
    return NULL;                            // task not found, return NULL
}

/**
 * function that is invoked once after the timer elapses
 *
 * @param timer struct timer_list ptr
 * @return void
 */
void timer_callback(struct timer_list *timer) {
    unsigned long flag;
    rms_task_struct_t *task;
    printk(KERN_ALERT "[KERN_ALERT]: TIMER CALLBACK\n");
    spin_lock_irqsave(&lock, flag);                                 // enter critical section
    task = container_of(timer, rms_task_struct_t, wakeup_timer);    // get the task
    __get_task_by_pid(task->pid)->state = READY;                    // set task->state to READY
    spin_unlock_irqrestore(&lock, flag);                            // exit critical section

    wake_up_process(dispatch_thread);                               // wake up dispatch_thread
}

/**
 * function that find the ready task
 * with the highest priority
 *
 * @param void
 * @return rms_task_struct_t *ptr
 */
rms_task_struct_t *get_highest_priority_ready_task(void) {
    int mutex_ret;
    rms_task_struct_t *tmp, *task = NULL;

    // Reference: https://www.kernel.org/doc/htmldocs/kernel-locking/API-mutex-lock-interruptible.html
    // Reference: https://elixir.bootlin.com/linux/latest/source/kernel/locking/mutex.c#L977
    mutex_ret = mutex_lock_interruptible(&task_list_mutex); // enter critical section
    list_for_each_entry(tmp, &rms_task_struct_list, list) {
        if (tmp->state != READY) continue;                  // skip NOT READY tasks
        if (task == NULL) {                                 // find first READY task
            task = tmp;
        } else {
            if (task->period > tmp->period) {               // replace READY task if higher priority
                task = tmp;
            }
        }
    }
    mutex_unlock(&task_list_mutex);                         // exit critical section

    return task;
}

/**
 * function that set task priority
 *
 * @param task      task that needs to set priority
 * @param priority  task priority
 */
void __set_priority(rms_task_struct_t *task, int policy, int priority) {
    // Reference: https://elixir.free-electrons.com/linux/v5.10.16/source/include/uapi/linux/sched/types.h#L100
    struct sched_attr sa;
    sa.sched_policy = policy;       // set task scheduling policy
    sa.sched_priority = priority;   // set task scheduling priority
    // Reference: https://elixir.free-electrons.com/linux/v5.10.16/source/include/linux/sched.h#L1692
    sched_setattr_nocheck(task->linux_task, &sa);
}

/**
 * function that handle dispatch_thread
 *
 * @param arg   void ptr
 * @return int  0-exit
 */
static int dispatch_thread_fn(void *arg) {
    int mutex_ret;
    rms_task_struct_t *task;

    while (1) {
        // put dispatching thread to sleep
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();

        if (kthread_should_stop()) return 0;                    // stop kthread

        mutex_ret = mutex_lock_interruptible(&cur_task_mutex);  // enter critical section
        // no higher priority task
        if ((task = get_highest_priority_ready_task()) == NULL) {
            // lower cur_task priority
            if (cur_task != NULL) {
                __set_priority(cur_task, SCHED_NORMAL, 0);
            }
        // exist higher priority task
        } else {
            // higher priority task preempt cur_task
            if (cur_task != NULL && task->period < cur_task->period) {
                cur_task->state = READY;
                __set_priority(cur_task, SCHED_NORMAL, 0);
            }
            // wake up highest priority task
            task->state = RUNNING;
            wake_up_process(task->linux_task);
            __set_priority(task, SCHED_FIFO, 99);
            cur_task = task;
        }
        mutex_unlock(&cur_task_mutex);                          // exit critical section
    }
}

/**
 * function to read /proc file
 *
 * @param *file     file to read
 * @param *buffer   user buffer
 * @param size      size of user buffer
 * @param *offl     offset in the file
 * @return ssize_t  number of byte copied
 */
static ssize_t proc_read(struct file *file, char __user *buffer, size_t size, loff_t *loff) {
    unsigned long cpy_kernel_byte;
    ssize_t byte_read = 0;
    int mutex_ret;
    char *kbuf;
rms_task_struct_t *task;
    if (*loff == 1) return 0;

    if (!access_ok(buffer, size)) {                         // check the access of the buffer
        printk(KERN_ALERT "[KERN_ALERT]: User Buffer is NOT WRITABLE\n");
        return -EINVAL;
    }

    kbuf = (char *)kmalloc(size, GFP_KERNEL);               // allocate memory in kernel
    if (kbuf == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to allocate memory in kernel\n");
        return -ENOMEM;
    }

    // get all the task info in the list
    mutex_ret = mutex_lock_interruptible(&task_list_mutex); // enter critical section
    list_for_each_entry(task, &rms_task_struct_list, list) {
        byte_read += sprintf(kbuf + byte_read, "%d: %lu, %lu\n", task->pid, task->period, task->computation);
    }
    mutex_unlock(&task_list_mutex);                         // exit critical section

    kbuf[byte_read] = '\0';                                 // null terminate kbuf

    cpy_kernel_byte = copy_to_user(buffer, kbuf, byte_read);
    if (cpy_kernel_byte != 0) {
        printk(KERN_ALERT "[KERN_ALERT]: copy_from_user fail\n");
    }

    kfree(kbuf);

    *loff = 1;
    return byte_read;
}

/**
 * function that check if utilization factor > = < utilization bound
 *
 * @param computation   computation time of the task
 * @param period        period of the task
 * @return int          1-success   0-fail
 */
int admission_ctrl(unsigned long computation, unsigned long period) {
    int mutex_ret;
    unsigned long util_factor = 0;
    rms_task_struct_t *task;

    // sum up utilization of existing tasks
    mutex_ret = mutex_lock_interruptible(&task_list_mutex); // enter critical section
    list_for_each_entry(task, &rms_task_struct_list, list) {
        util_factor += (task->computation * MULTIPLIER) / task->period;
    }
    mutex_unlock(&task_list_mutex);                         // exit critical section

    // add utilization of new task
    util_factor += (computation * MULTIPLIER) / period;

    if(util_factor <= UTIL_BOUND) return 1;                 // util_factor <= UB
    return 0;                                               // unable to schedule new task
}

/**
 * function that register process
 *
 * @param   buf
 * @return  void
 */
void register_process(char *buf) {
    int mutex_ret;
    rms_task_struct_t *task;

    // allocate memory by cache for tmp
    task = (rms_task_struct_t *)kmem_cache_alloc(rms_task_struct_cache, GFP_KERNEL);

    INIT_LIST_HEAD(&(task->list));                          // init list
    sscanf(strsep(&buf, ","), "%d", &task->pid);            // set task->pid
    sscanf(strsep(&buf, ","), "%lu", &task->period);        // set task->period
    sscanf(strsep(&buf, "\n"), "%lu", &task->computation);  // set task->computation
    printk(KERN_ALERT "[KERN_ALERT]: REGISTER PROCESS WITH PID: %d C: %lu P: %lu\n", task->pid, task->computation, task->period);
    task->deadline = 0;                                     // set task->deadline
    task->linux_task = find_task_by_pid(task->pid);         // set task->linux_task
    timer_setup(&(task->wakeup_timer), timer_callback, 0);  // set task->wakeup_timer

    // check for admission_control
    if (!admission_ctrl(task->computation, task->period)) {
        return;
    }

    mutex_ret = mutex_lock_interruptible(&task_list_mutex); // enter critical section
    list_add(&(task->list), &rms_task_struct_list);         // add task to rms_task_struct_list
    mutex_unlock(&task_list_mutex);                         // exit critical section
}

/**
 * function that yield process
 *
 * @param buf buf contains process info
 * @return void
 */
void yield_process(char *buf) {
    pid_t pid;
    int mutex_ret;
    rms_task_struct_t *task;
    int wakeup;

    sscanf(buf, "%d", &pid);                                // set task->pid
    printk(KERN_ALERT "[KERN_ALERT]: YIELD PROCESS WITH PID: %d\n", pid);

    mutex_ret = mutex_lock_interruptible(&task_list_mutex); // enter critical section
    task = __get_task_by_pid(pid);                          // get the task struct
    mutex_unlock(&task_list_mutex);                         // exit critical section

    if (task->deadline == 0) {
        wakeup = 0;
        task->deadline = jiffies + msecs_to_jiffies(task->period);
    } else {
        // update task->deadline
        task->deadline += msecs_to_jiffies(task->period);
        // check if the task should wake up
        wakeup = jiffies > task->deadline ? 1 : 0;
    }

    if (wakeup) return;

    mod_timer(&(task->wakeup_timer), task->deadline);       // update timer with task->deadline
    task->state = SLEEPING;                                 // set task->state to SLEEPING
    mutex_ret = mutex_lock_interruptible(&cur_task_mutex);  // enter critical section
    cur_task = NULL;                                        // reset cur_task to NULL
    mutex_unlock(&cur_task_mutex);                          // exit critical section
    wake_up_process(dispatch_thread);                       // wake up scheduler
    set_current_state(TASK_UNINTERRUPTIBLE);                // sleep
    schedule();
}

/**
 * function that de-register process
 *
 * @param *buf  buf contains process info
 * @return void
 */
void deregister_process(char *buf) {
    pid_t pid;
    int mutex_ret;
    rms_task_struct_t *task;

    sscanf(buf, "%d", &pid);                    // get task pid from buf
    printk(KERN_ALERT "[KERN_ALERT]: DEREGISTER PROCESS WITH PID: %d\n", pid);

    mutex_ret = mutex_lock_interruptible(&task_list_mutex); // enter critical section
    task = __get_task_by_pid(pid);                          // find task from task_struct_list
    del_timer(&task->wakeup_timer);                         // delete timer of the task
    list_del(&task->list);                                  // delete task from task_struct_list
    mutex_unlock(&task_list_mutex);                         // exit critical section

    mutex_ret = mutex_lock_interruptible(&cur_task_mutex);  // enter critical section
    if (cur_task == task) {                                 // check if cur task is one to delete
        cur_task = NULL;                                    // set cur task to NULL
        wake_up_process(dispatch_thread);                   // wake up process
    }
    mutex_unlock(&cur_task_mutex);                          // exit critical section

    kmem_cache_free(rms_task_struct_cache, task);           // free cache
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
static ssize_t proc_write(struct file *file, const char __user *buffer, size_t size, loff_t *loff) {
    unsigned long cpy_usr_byte;
    char *kbuf;

    if (!access_ok(buffer, size)) {                 // check the access of the buffer
        printk(KERN_ALERT "[KERN_ALERT]: Buffer is NOT READABLE\n");
        return -EINVAL;
    }

    kbuf = (char *)kmalloc(size+1, GFP_KERNEL);     // allocate memory in kernel
    if (kbuf == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to allocate memory in kernel\n");
        return -ENOMEM;
    }

    cpy_usr_byte = copy_from_user(kbuf, buffer, size);
    if (cpy_usr_byte != 0) {
        printk(KERN_ALERT "[KERN_ALERT]: copy_from_user fail\n");
    }

    kbuf[size] = '\0';                              // null terminate kbuf

    switch (kbuf[0]) {
        case REGISTRATION:                          // register process
            register_process(kbuf+PID_OFFSET);
            break;
        case YIELD:                                 // yield process
            yield_process(kbuf+PID_OFFSET);
            break;
        case DE_REGISTRATION:                       // de-register process
            deregister_process(kbuf+PID_OFFSET);
            break;
        default:
            printk(KERN_ALERT "[KERN_ALERT]: Task Status Not Found\n");
    }

    kfree(kbuf);

    return size;
}

/**
 * called when mp2 module is loaded
 *
 * @param void
 * @return int  0-success, other-failed
 */
static int __init mp2_init(void) {
    #ifdef DEBUG
    printk(KERN_ALERT "[KERN_ALERT]: MP2 MODULE LOADING\n");
    #endif

    // create /proc/mp2 dir
    if ((proc_dir = proc_mkdir(DIRECTORY, NULL)) == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to create /proc/" DIRECTORY "\n");
        return -ENOMEM;
    }
    // create /proc/mp2/status
    if ((proc_entry = proc_create(FILENAME, 0666, proc_dir, &proc_fops)) == NULL) {
        remove_proc_entry(DIRECTORY, NULL);
        printk(KERN_ALERT "[KERN_ALERT]: Fail to create /proc/" DIRECTORY "/" FILENAME "\n");
        return -ENOMEM;
    }

    // init spinlock
    spin_lock_init(&lock);

    // create cache
    rms_task_struct_cache = KMEM_CACHE(rms_task_struct , SLAB_PANIC);

    // init and run the dispatching thread
    dispatch_thread = kthread_run(dispatch_thread_fn, NULL, "dispatch_thread");

    printk(KERN_ALERT "[KERN_ALERT]: MP2 MODULE LOADED\n");
    return 0;
}

/**
 * called when mp2 module is unloaded
 *
 * @param void
 * @return void
 */
static void __exit mp2_exit(void) {
    rms_task_struct_t *pos, *n;

    #ifdef DEBUG
    printk(KERN_ALERT "[KERN_ALERT]: MP2 MODULE UNLOADING\n");
    #endif

    remove_proc_entry(FILENAME, proc_dir);  // remove /proc/mp2/status
    remove_proc_entry(DIRECTORY, NULL);     // remove /proc/mp2
    mutex_destroy(&task_list_mutex);        // destroy task_list_mutex
    mutex_destroy(&cur_task_mutex);         // destroy cur_task_mutex

    kthread_stop(dispatch_thread);          // stop the dispatching thread

    // free rms_task_struct_list
    list_for_each_entry_safe(pos, n, &rms_task_struct_list, list) {
        list_del(&pos->list);                           // delete rms_task_struct_list
        kmem_cache_free(rms_task_struct_cache, pos);    // free rms_task_struct_list
    }

    kmem_cache_destroy(rms_task_struct_cache);          // destroy the rms_task_struct_cache

    printk(KERN_ALERT "[KERN_ALERT]: MP2 MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(mp2_init);
module_exit(mp2_exit);