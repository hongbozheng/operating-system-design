# Virtual Memory Page Fault Profiler

Build a kernel-level module that harvests the page fault and utilization 
information of registered tasks and exposes them by using a memory buffer 
that is directly mapped into the virtual address space of the monitor process.

## Project Setup
This project depends on the installation of the following essential packages:

None

## Build Project
With the project setup, build the command-line application as follows:
```
make
```

To clean up the project.
```
make clean
```

## Test Profiler Kernel Module
Run the following command.
```
./get_pro_data.bash
```

## Case Study Analysis

### Case Study 1: Thrashing & Locality
- `Work process 1`: 1024MB Memory, Random Access, and 50,000 accesses per 
iteration.
- `Work process 2`: 1024MB Memory, Random Access, and 10,000 accesses per 
iteration.

- `Work process 3`: 1024MB Memory, Random Locality Access, and 50,000 accesses 
per iteration.
- `Work process 4`: 1024MB Memory, Locality-based Access, and 10,000 accesses 
per iteration.

_Work process 1 & work process 2_ has more page fault compared to 
_work process 3 & work process 4_ because they use random access memory. 
In addition, _work process 4_ uses local access memory. Therefore, 
_work process 3 & work process 4_ have less page fault, and they have faster 
time.

### Case Study 2: Multiprogramming
`NUM_CORE: 4, MEM: 4GB, SWAP: 1G`
- `Work process 5`: 200MB Memory, Random Locality Access, and 10,000 accesses 
per iteration.

From `N=1` to `N=20`, CPU utilization increases as the number of processes (N) 
increases. However, the CPU utilization decreases during thrashing which is 
cased by page swaps.

`Case Study 1` data in `Case_Study_1_Profile_Data` folder.
- profile1.data
- profile2.data

`Case Study 2` data in `Case_Study_2_Profile_Data` folder.
- 

Figures are in `Case_Study_Analysis` folder

## Design

### mp3.h & mp3.c

1. Initialize `struct proc_dir_entry`
- `proc_dir` for directory.
- `proc_entry` for status file.

2. Create process directory and file
- Create `/proc/mp3` directory.
- Create `/proc/mp3/status` file.

3. Initialize `spinlock`
- `spinlock` provides protection to `work_proc_struct_list`.
- Add, delete, or modify `work_proc_struct`.

4. Initialize Process Control Block (PCB)
- `typedef struct work_proc_struct_t` PCB Struct.
    - `struct task_struct -linux_task` define in \<linux/sched.h\>.
    - `struct list_head list` list for `work_proc_struct`.
    - `pid_t pid` process ID.
    - `unsigned long utilization` process utilization in _jiffies_.
    - `unsigned long maj_page_flt` major page fault.
    - `unsigned long min_page_flt` minor page fault.

5. Initialize other module
- `DELAYED_WORK` & `update_data` (work function).
- `struct workqueue_struct -wq` work_queue struct.
- `unsigned long delay_jiffies` delay for delayed_work in _jiffies_.
- `unsigned long -prof_buf` profiler buffer.
- `unsigned long prof_buf_ptr` profiler buffer ptr.
- SetPageReserved

6. Create `struct proc_ops`
- `proc_read` process read from file.
- `proc_write`  process write to file.

7. `proc_read` function
- If `loff_t - offset > 0` the user already read the file, `return 0`.
- Else, start reading from `/proc/mp2/status` file, and modify the 
`loff_t -offset = byte_read`.
- Use `sprintf` to read task pid and store it in `work_proc_struct` in the task 
struct list.
    - `<pid>`

8. `__get_work_proc_by_pid` function
- Find the task in the `work_proc_struct_list` with given `pid_t pid`.
  - If task exists, return `proc_work_struct`.
  - Else return `NULL`.

9. `register_process` function
- Use `sscanf` to get the `work_proc->pid`.
- Call `__get_work_proc_by_pid` to find the `work_proc_struct`.
- Init `work_proc_struct` and add to the `work_proc_struct_list`.

10. `deregister_process` function
- Use `sscanf` to get the `work_proc->pid`.
- Call `__get_work_proc_by_pid` to get the `work_proc struct` with given 
`pid_t pid`.
- Delete the `work_proc_struct` from the task struct list.
- If it's tha last `work_proc_struct`, call `cancel_delayed_work_sync` to wait 
and cancel the delayed work.

11. `proc_write` function
- In `mp1.c`, store `copy_from_user` information into `kbuf`.
- Call different functions based on the first letter ( kbuf [0] ).
  - `REGISTRATION` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; -----> `register_process`.
  - `DE_REGISTRATION` -----> `deregister_process`.

12. `cdev_mmap` function
- Mapping the physical pages of the buffer to the virtual address space of a requested user process

13. `update_data` function
- Work function to update data.
- Loop through each `work_proc_struct` in the `work_proc_struct_list`.
  - Ccall `get_cpu_use` to get all the data.
  - Sum up the total utilization, total major page fault, and total minor page 
  fault.
- Write them into profiler buffer.
- Enqueue `delayed_work` with delay.

14. exit
- Remove `proc_entry` which is `/proc/mp1/status` file.
- Remove `proc_dir` which is `/proc/mp1` directory.
- Delete `cdev`.
- Unregister `dev`.
- If there's still delayed work pending.
  - Call `cancel_delayed_work_sync` to wait and cancel the `delayed work`.
- Destroy workqueue.
- Loop through `work_proc_struct_list` to delete and free the list.
- ClearPageReserved.
