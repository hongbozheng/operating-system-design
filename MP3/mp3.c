#define LINUX

#include "mp3.h"

unsigned long delay;

// linked list which will be used to store all the task
//struct list_head work_proc_struct_list;

void profiler_work_function(struct work_struct *work);
DECLARE_DELAYED_WORK(profiler_work, &profiler_work_function);




unsigned vbuf_ptr = 0;

int mp3_cdev_open(struct inode* inode, struct file* file) 
{
  return 0;
}

int mp3_cdev_close(struct inode* inode, struct file* file) 
{
  return 0;
}

// cdev mmap method, map each vmalloced space to device space,
// so that it can be contiguous
static int mp3_cdev_mmap(struct file *f, struct vm_area_struct *vma)
{
	unsigned long index = 0;
   unsigned long pfn;
   unsigned long size = vma->vm_end - vma->vm_start;
	printk(KERN_INFO "mp3: receive mmap request\n");

	if (vma->vm_end - vma->vm_start > MAX_VBUFFER) {
		printk(KERN_WARNING "mp3: mmap length exceed max vbuf\n");
		return -1;
	}
   
   for (index = 0; index < size; index +=PAGE_SIZE){
      pfn = vmalloc_to_pfn((void *)(((unsigned long)vbuf) + index));
		if(remap_pfn_range(vma, vma->vm_start+index, pfn, PAGE_SIZE, vma->vm_page_prot)){
			printk(KERN_ALERT "MP3 module could not perform mmap!\n");
			return -1;
		}
	}
	return 0;
}

/*Define the behavior when the proc file is read by the user space program*/
static ssize_t proc_read (struct file *file, char __user *buffer, size_t size, loff_t *offset) {
   unsigned long cpy_kernel_byte;
   char *kbuf;
   int copied = 0;
    work_proc_struct_t* cur;
   printk(KERN_DEBUG "mp3: receive read request\n");
   // check reenter
   if(*offset > 0) return 0;

   // allocate memory and init it
   kbuf = (char *)kmalloc(MAX_BUF * sizeof(char), GFP_KERNEL);
   memset(kbuf, 0, MAX_BUF * sizeof(char));

   spin_lock(&lock);
   // todo
   list_for_each_entry(cur, &work_proc_struct_list, list_node){
        // traversal all the node in the list and write to the buffer
        copied += sprintf(kbuf + copied, "PID: %d\n", cur->pid);
        printk(KERN_DEBUG "mp3: writing PID: %d\n", cur->pid);
   }
   spin_unlock(&lock);
   // add end sign
   kbuf[copied] = '\0';
   printk(KERN_DEBUG "mp3: read callback with content %s\n", kbuf);
   // copy the result to the user space buffer
   cpy_kernel_byte = copy_to_user(buffer, kbuf, copied);
   kfree(kbuf);
   *offset = copied;
   return copied;
}


void profiler_work_function(struct work_struct *work) {
    work_proc_struct_t *cur;
   unsigned long major_fault_sum, minor_fault_sum, utilization_sum, utime, stime;
   major_fault_sum = minor_fault_sum = utilization_sum = 0;

   spin_lock(&lock);
   list_for_each_entry(cur, &work_proc_struct_list, list_node){
        // traversal all the node in the list and write to the buffer
      if(0 == get_cpu_use(cur->pid, &cur->minor_page_fault, &cur->major_page_fault, &utime, &stime)){
         // get data success
		   major_fault_sum += cur->major_page_fault;
         minor_fault_sum += cur->minor_page_fault;
         cur->utilization = utime + stime;
         utilization_sum += cur->utilization;
		}
   }
   spin_unlock(&lock);
   // save the information to the memory buffer
	vbuf[vbuf_ptr++] = jiffies;
   vbuf[vbuf_ptr++] = minor_fault_sum;
   vbuf[vbuf_ptr++] = major_fault_sum;
   vbuf[vbuf_ptr++] = utilization_sum;

   if (vbuf_ptr >= MAX_VBUFFER) {
      printk(KERN_DEBUG "mp3: profile vmalloc buffer full, warp around\n");
      vbuf_ptr = 0;
   }
   if (work_queue) {
		queue_delayed_work(work_queue, &profiler_work, delay);
	}
}

/*check whether the target task exists in the task linked list*/
int check_process_exist(int pid){
    work_proc_struct_t* cur;
    work_proc_struct_t* tmp;
    // indicate whether we find the task
    int flag = 0;
    // lock the spinning lock
    spin_lock(&lock);
    // travesal all the node
    list_for_each_entry_safe(cur, tmp, &work_proc_struct_list, list_node){
        //check the pid
        if(cur->pid == pid){
            flag = 1;
            break;
        }
    }
    spin_unlock(&lock);
    return flag;
}

work_proc_struct_t *__get_work_proc_by_pid(pid_t pid) {
    work_proc_struct_t *work_proc = NULL;
    list_for_each_entry(work_proc, &work_proc_struct_list, list_node) {
        if (work_proc->pid == pid) return work_proc;
    }
    return work_proc;
}

/*Define the behavior when we register the target task*/
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

    if (work_queue == NULL) {
        work_queue = create_singlethread_workqueue("workqueue");
    }
    queue_delayed_work(work_queue, &profiler_work, delay);

    spin_lock_irqsave(&lock, flag);
    // Reference: https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch10s05.html
    list_add(&work_proc->list_node, &work_proc_struct_list);
    spin_unlock_irqrestore(&lock, flag);

    return 1;
}

/*Define the behavior when we unregister the target task*/
int dereg_proc(char *buf){
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

    if (list_empty(&work_proc_struct_list)) {
        cancel_delayed_work(&profiler_work);
        flush_workqueue(work_queue);
        destroy_workqueue(work_queue);
        work_queue = NULL;
        printk(KERN_INFO "mp3: deleted work queue");
    }

    return 1;
}

/*Define the behavior when the proc file is wriiter by the user space program*/
static ssize_t proc_write (struct file *file, const char __user *buffer, size_t size, loff_t *offset) {
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
   int index = 0;
   int ret = 0;
   #ifdef DEBUG
   printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE LOADING\n");
   #endif
   // Insert your code here ...

    if ((proc_dir = proc_mkdir(DIRECTORY, NULL)) == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to create /proc/" DIRECTORY "\n");
        return -ENOMEM;
    }

    if ((proc_entry = proc_create(FILENAME, 0666, proc_dir, &proc_fops)) == NULL) {
        remove_proc_entry(DIRECTORY, NULL);
        printk(KERN_ALERT "[KERN_ALERT]: Fail to create /proc/" DIRECTORY "/" FILENAME "\n");
        return -ENOMEM;
    }

    delay = msecs_to_jiffies(DELAY);

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

    if ((vbuf = vmalloc(MAX_VBUFFER)) == NULL) {
        printk(KERN_ALERT "[KERN_ALERT]: Fail to allocate virtually contiguous memory\n");
        ret = -ENOMEM;
        goto rm_wq;
    }
    memset(vbuf, -1, MAX_VBUFFER);

    work_queue = NULL;

    // Reference: https://linux-kernel-labs.github.io/refs/heads/master/labs/memory_mapping.html
    for(index = 0; index < MAX_VBUFFER; index+=PAGE_SIZE){
        SetPageReserved(vmalloc_to_page((void *)(((unsigned long)vbuf) + index)));
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
void __exit mp3_exit(void)
{
    unsigned long flag;
    work_proc_struct_t *pos, *n;
    int index = 0;
    #ifdef DEBUG
    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE UNLOADING\n");
    #endif
    // Insert your code here ...

    remove_proc_entry(FILENAME, proc_dir);
    remove_proc_entry(DIRECTORY, NULL);

    // Reference: https://www.kernel.org/doc/htmldocs/kernel-api/API-cdev-del.html
    cdev_del(&cdev);
    unregister_chrdev_region(dev, 1);

    if (work_queue != NULL){
        cancel_delayed_work(&profiler_work);
		flush_workqueue(work_queue);
		destroy_workqueue(work_queue);
        work_queue = NULL;
    }

    spin_lock_irqsave(&lock, flag);
    // delete all the entries in the list
    list_for_each_entry_safe(pos, n, &work_proc_struct_list, list_node) {
        list_del(&pos->list_node);
        kfree(pos);
    }
    spin_unlock_irqrestore(&lock, flag);

    // Reference: https://linux-kernel-labs.github.io/refs/heads/master/labs/memory_mapping.html
    for(index = 0; index < MAX_VBUFFER; index+=PAGE_SIZE){
        ClearPageReserved(vmalloc_to_page((void *)(((unsigned long)vbuf) + index)));
    }

    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp3_init);
module_exit(mp3_exit);