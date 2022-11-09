# MP2 - Virtual Memory Page Fault Profiler
##### Instructor: Tianyin Xu
Deadline --- Friday, November, 11th, 11:59PM

Build a kernel-level module that harvests the page fault and utilization information of registered tasks and exposes
them by using a memory buffer that is directly mapped into the virtual address space of the monitor process
---

## Project Setup
This project depends on the installation of the following essenstial packages:

None
---

## Build Project
With the project setup, build the command-line application as follow:
```
make
```

To clean up the project
```
make clean
```
---

## Test Profiler Kernel Module
Run the following command
```
./get_pro_data.bash
```
---

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

##### 6. Create `struct proc_ops`
* `proc_read` process read from file
* `proc_write`  process write to file

##### 7. `proc_read` function
* check if the user already read all the file
* if not, start reading from `/proc/mp2/status` file, and modify the `loff_t *loff`
* else return `0` for read 0 byte
* use `sprintf` to read task information and store them in `rms_task_struct` in the task struct list
    * `<pid>: <task_period>, <computation_time>`

##### 8. `proc_write` function
* in `mp1.c`, store `copy_from_user` information into `kbuf`
* call different functions based on the first letter ( kbuf [0] )
    * `REGISTRATION` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; -----> `register_process`
    * `YIELD` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; -----> `yield_process`
    * `DE_REGISTRATION` -----> `deregister_process`

##### 9. `admission_control` function
* compute the `utilization_factor` sum of all the existing tasks in the task struct list
* add the `utilization_factor` of the new task
* if it's over `Utilization Bound` ignore the new task

##### 10. `__get_task_by_pid` function
* find the task in the task struct list with `pid` and return task

##### 11. `timer_callback` function
* use `container_of` find the `rms_task_struct` from the task struct list
* set the `task->state` to `READY`

##### 12. `register_process` function
* use `sscanf` and `strsep` to get the `task->pid`, `task->period`, and `task->computation`
* call `admission_control` function to check `Utilization_Bound`
* add `rms_task_struct *task` to task struct list

##### 13. `yield_process` function
* check `task->deadline`
* if `task->deadline` is passed, call `wake_up_process` to wake up the task

##### 14. `deregister_process` function
* delete the task from the task struct list, and delete `task->wakeup_timer`
* if it's also the `cur_task`, set `cur_task` to NULL, and call `wake_up_process`

##### 15. `get_highest_priority_ready_task` function
* loop through task struct list and find the `READY` task with the `highest priority`

##### 16. `__set_priority` function
* set the `policy` & `priority` of the task

##### 17. `dispatch_thread_fn` function
* check if the kthread should stop by calling `kthread_should_stop`
* get the `READY` task with the `highest priority`
* if NO `READY` task, lower the priority of the `cur_task`
* else `cur_task` is preempted by `READY` task
* call `wake_up_process` on that task

##### 18. exit
* remove `proc_entry` which is `/proc/mp1/status` file
* remove `proc_dir` which is `/proc/mp1` directory
* destroy 2 `mutex`
* stop kernel thread `dispatch_thread`
* delete & free the memory of the linked list with `list_for_each_entry_safe` macro
* free kernel memory cache
* destroy `struct kem_cache *rms_task_struct_cache`

#### userapp.h & userapp.c

##### 1. `register_process` function
* open `/proc/mp1/status` file and write `R, <pid>, <period>, <computation_time>`

##### 2. `process_exist` function
* open `/proc/mp1/status` file and check if `pid` exists

##### 3. `yield_process` function
* open `/proc/mp1/status` file and write `Y, <pid>`

##### 4. `deregister_process` function
* open `/proc/mp1/status` file and write `D, <pid>`

##### 5. `main` function
* loop `do_job` function with `freq` to simulate periodic task

## Developers
* Hongbo Zheng [NetID: hongboz2]