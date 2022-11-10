# MP3 - Virtual Memory Page Fault Profiler
##### Instructor: Tianyin Xu
Deadline --- Friday, November, 11th, 11:59PM

Build a kernel-level module that harvests the page fault and utilization information of registered tasks and exposes
them by using a memory buffer that is directly mapped into the virtual address space of the monitor process

## Project Setup
This project depends on the installation of the following essenstial packages:

None

## Build Project
With the project setup, build the command-line application as follow:
```
make
```

To clean up the project
```
make clean
```

## Test Profiler Kernel Module
Run the following command
```
./get_pro_data.bash
```

## Design

#### mp3.h & mp3.c

##### 1. Initialize `struct proc_dir_entry`
* `proc_dir` for directory
* `proc_entry` for status file

##### 2. Create process directory and file
* create `/proc/mp3` directory
* create `/proc/mp3/status` file

##### 3. Initialize `spinlock`
* `spinlock` provides protection to `work_proc_struct_list`
* Add, Delete, or Modify `work_proc_struct`

##### 4. Initialize Process Control Block (PCB)
* `typedef struct work_proc_struct_t` PCB Struct
    * `struct task_struct *linux_task` define in \<linux/sched.h\>
    * `struct list_head list` list for `work_proc_struct`
    * `pid_t pid` process ID
    * `unsigned long utilization` process utilization in _jiffies_
    * `unsigned long maj_page_flt` major page fault
    * `unsigned long min_page_flt` minor page fault

##### 5. Initialize other module
* `DELAYED_WORK` & `update_data` (work function)
* `struct workqueue_struct *wq` work_queue struct
* `unsigned long delay_jiffies` delay for delayed_work in jiffies
* `unsigned long *prof_buf` profiler buffer
* `unsigned long prof_buf_ptr` profiler buffer ptr
* SetPageReserved

##### 6. Create `struct proc_ops`
* `proc_read` process read from file
* `proc_write`  process write to file

##### 7. `proc_read` function
* `if loff_t *offset > 0` the user already read the file, `return 0`
* else, start reading from `/proc/mp2/status` file, and modify the `loff_t *offset = byte_read`
* use `sprintf` to read task pid and store it in `work_proc_struct` in the task struct list
    * `<pid>`

##### 8. `__get_work_proc_by_pid` function
* find the task in the `work_proc_struct_list` with given `pid_t pid`
  * if task exists, return `proc_work_struct`
  * else return `NULL`

##### 9. `register_process` function
* use `sscanf` to get the `work_proc->pid`
* call `__get_work_proc_by_pid` to find the `work_proc_struct`
* init `work_proc_struct` and add to the `work_proc_struct_list`

##### 10. `deregister_process` function
* use `sscanf` to get the `work_proc->pid`
* call `__get_work_proc_by_pid` to get the `work_proc struct` with given `pid_t pid`
* delete the `work_proc_struct` from the task struct list
* if it's tha last `work_proc_struct`, call `cancel_delayed_work_sync` to wait and cancel the delayed work

##### 11. `proc_write` function
* in `mp1.c`, store `copy_from_user` information into `kbuf`
* call different functions based on the first letter ( kbuf [0] )
  * `REGISTRATION` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; -----> `register_process`
  * `DE_REGISTRATION` -----> `deregister_process`

##### 12. `cdev_mmap` function
* mapping the physical pages of the buffer to the virtual address space of a requested user process

##### 13. `update_data` function
* work function to update data
* loop through each `work_proc_struct` in the `work_proc_struct_list`
  * call `get_cpu_use` to get all the data
  * sum up the total utilization, total major page fault, and total minor page fault
* write them into profiler buffer
* enqueue `delayed_work` with delay

##### 14. exit
* remove `proc_entry` which is `/proc/mp1/status` file
* remove `proc_dir` which is `/proc/mp1` directory
* delete `cdev`
* unregister `dev`
* if there's still delayed work pending
  * call `cancel_delayed_work_sync` to wait and cancel the `delayed work`
* destroy workqueue
* loop through `work_proc_struct_list` to delete and free the list
* ClearPageReserved

## Developers
* Hongbo Zheng [NetID: hongboz2]