#define LINUX

#include "mp3.h"

// max buf size
#define MAX_BUF 4096
// file and dir name
#define MAX_VBUFFER (4 * 128 * 1024)

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

/*Define the behavior when we register the target task*/
int registeration(int pid){
// check whether the task exist in the list
   int exist = check_process_exist(pid);
   if(exist){
      // already in the list, do nothing
      printk(KERN_WARNING "mp3: registeration already exist entry for pid %d\n", pid);
      return -1;
   }

   // allocate new memory space
    work_proc_struct_t* process = (work_proc_struct_t *)kmalloc(sizeof(work_proc_struct_t), GFP_KERNEL);
   if(!process)
   // check whether successfully allocate memory space
   {
      printk(KERN_WARNING "mp3: registeration malloc for new PCB failed\n");
      return 0;
   }
   printk(KERN_DEBUG "mp3: registeration for pid %d\n", pid);
   // get the linux task structure
   process->linux_task = find_task_by_pid(pid);
   if(process->linux_task == NULL){
      printk(KERN_WARNING "mp3: registeration cannot find task with pid %d\n", pid);
      return -1;
   }
   process->pid = pid;
   process->major_page_fault = process->minor_page_fault = 0;
   process->utilization = 0;

   if(work_queue == NULL){
       work_queue = create_singlethread_workqueue("workqueue");
   }
   queue_delayed_work(work_queue, &profiler_work, delay);
   spin_lock(&lock);
   // add the current task to the linked list
   list_add(&process->list_node, &work_proc_struct_list);
   spin_unlock(&lock);
   return 0;
}

/*Define the behavior when we unregister the target task*/
int unregisteration(int pid){
    work_proc_struct_t* cur;
    work_proc_struct_t* tmp;
   int flag = 0;
   printk(KERN_DEBUG "mp3: unregisteration for pid %d\n", pid);
   // traversal all the node
   list_for_each_entry_safe(cur, tmp, &work_proc_struct_list, list_node)
   {
      if (pid == cur->pid){
         spin_lock(&lock);
         // remove list head
         list_del(&cur->list_node);
         // free the memory
         kfree(cur);
         printk(KERN_DEBUG "mp3: deregisteration delete for pid %d\n", pid);
         spin_unlock(&lock);
         flag = 1;
      }
   }
   if (list_empty(&work_proc_struct_list)) {
		cancel_delayed_work(&profiler_work);
		flush_workqueue(work_queue);
		destroy_workqueue(work_queue);
      work_queue = NULL;
		printk(KERN_INFO "mp3: deleted work queue");
    }
    if(!flag){
        printk(KERN_WARNING "mp2: deregisteration cannot find pid %d\n", pid);
    }
    return 0;
}

/*Define the behavior when the proc file is wriiter by the user space program*/
static ssize_t proc_write (struct file *file, const char __user *buffer, size_t size, loff_t *data) {
   char* buf;
   char cmd;
   int cpy = 0;
   // allocate the space and init it used to get info from user space
   buf = (char *)kmalloc(MAX_BUF * sizeof(char), GFP_KERNEL);
   memset(buf, 0, MAX_BUF * sizeof(char));
   cpy = copy_from_user(buf, buffer, size);
   // add end sign
   buf[size] = '\0';
   printk(KERN_DEBUG "mp3: receive from user space with str %s\n", buf); 
   cmd = buf[0];
   switch (cmd)
   {
       case 'R':{
            // register request
            int pid;
            sscanf(buf, "R %d", &pid);
            printk(KERN_DEBUG "mp3: registeration pid: %d\n", pid); 
            // do the register operation
            registeration(pid);
       }
       break;
       case 'U':{
            // unregister request
            int pid;
            sscanf(buf, "U %d", &pid);
            printk(KERN_DEBUG "mp3: unregisteration pid: %d\n", pid); 
            // do the register operation
            unregisteration(pid);
       }
       break;
       default:{
            // illegal command
            printk(KERN_WARNING "mp3: receive illegal request\n"); 
       }
       break;
   }
   kfree(buf);
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
    work_proc_struct_t* cur;
    work_proc_struct_t* tmp;
    int index = 0;
    #ifdef DEBUG
    printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE UNLOADING\n");
    #endif
    // Insert your code here ...

    if (work_queue != NULL){
        cancel_delayed_work(&profiler_work);
		flush_workqueue(work_queue);
		destroy_workqueue(work_queue);
        work_queue = NULL;
    }

   spin_lock(&lock);
   // delete all the entries in the list
   list_for_each_entry_safe(cur, tmp, &work_proc_struct_list, list_node){
         printk(KERN_DEBUG "mp3: remove pid: %d from list \n", cur->pid);
         //remove list head
         list_del(&cur->list_node);
         // free the memory
         kfree(cur);
   }
   spin_unlock(&lock);
   for(index = 0; index < MAX_VBUFFER; index+=PAGE_SIZE){
      ClearPageReserved(vmalloc_to_page((void *)(((unsigned long)vbuf) + index)));
	}
   // Reference: https://www.kernel.org/doc/htmldocs/kernel-api/API-cdev-del.html
   cdev_del(&cdev);
	unregister_chrdev_region(dev, 1);
   remove_proc_entry(FILENAME, proc_dir);
   remove_proc_entry(DIRECTORY, NULL);

   printk(KERN_ALERT "[KERN_ALERT]: MP3 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp3_init);
module_exit(mp3_exit);