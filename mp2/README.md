# Rate-Monotonic CPU Scheduling

Implement a Real-Time CPU scheduler based on the Rate-Monotonic Scheduler (RMS) 
for a Single-Core Processor.

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

## Test Linux Kernel Module
### Test with `1 Periodic Task`
Run the following command.
```
./test_kernel_module_1.bash
```

### Test with `3 Periodic Tasks`
Run the following command.
```
./test_kernel_module_3.sh
```

## Design

### mp2.h & mp2.c

1. Initialize `struct proc_dir_entry`
- `proc_dir` for directory.
- `proc_entry` for status file.

2. Create process directory and file
- Create `/proc/mp2` directory
- Create `/proc/mp2/status` file

3. Initialize `spinlock` & `mutex`
- `spinlock` write `task->state` protection from in function `timer_callback`.
- 2 `mutex` write protection.
  - 1 mutex to protect write to `rms_task_struct_list`.
  - 1 mutex to protect write to `rms_task_struct cur_task`.

4. Initialize Process Control Block (PCB)
- `typedef struct rms_task_struct` PCB Struct.
  - `struct task_struct -linux_task` define in \<linux/sched.h\>.
  - `struct timer_list wakeup_timer` timer interrupt handler change state of the 
  `task->state` to READY after the wakeup timer expires.
  - `struct list_head list` list for `rms_task_struct`.
  - `pid_t pid` process ID.
  - `unsigned long period` task period in _ms_.
  - `unsigned long computation` task computation time in _ms_.
  - `unsigned long deadline` task deadline in _jiffies_.
  - `unsigned int state` task state.
    - `SLEEPING` 1
    - `READY` 2
    - `RUNNING` 3

5. Initialize other module
- Global `rms_task_struct -cur_task` store current running task.
- `struct task_struct -dispatch thread` responsible for triggering the context 
switches as needed.
- `struct kmem_cache -rms_task_struct_cache` used by the registration function 
to allocate a new instance of `rms_task_struct`.

6. Create `struct proc_ops`
- `proc_read` process read from file.
- `proc_write` process write to file.

7. `proc_read` function
- Check if the user already read all the file
- If not, start reading from `/proc/mp2/status` file, and modify the 
`loff_t -loff`.
- Else return `0` for read 0 byte.
- Use `sprintf` to read task information and store them in `rms_task_struct` 
in the task struct list.
  - `<pid>: <task_period>, <computation_time>`.

8. `proc_write` function
- In `mp1.c`, store `copy_from_user` information into `kbuf`.
- Call different functions based on the first letter ( kbuf [0] ).
  - `REGISTRATION` &nbsp;&nbsp;&nbsp;&nbsp; -----> `register_process`
  - `YIELD` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; -----> `yield_process`
  - `DE_REGISTRATION` -----> `deregister_process`

9. `admission_control` function
- Compute the `utilization_factor` sum of all the existing tasks in the task 
struct list.
- Add the `utilization_factor` of the new task.
- If it's over `Utilization Bound` ignore the new task.

10. `__get_task_by_pid` function
- Find the task in the task struct list with `pid` and return task.

11. `timer_callback` function
- Use `container_of` find the `rms_task_struct` from the task struct list.
- Set the `task->state` to `READY`.

12. `register_process` function
- Use `sscanf` and `strsep` to get the `task->pid`, `task->period`, 
and `task->computation`
- Call `admission_control` function to check `Utilization_Bound`.
- Add `rms_task_struct -task` to task struct list.

13. `yield_process` function
- Check `task->deadline`.
- If `task->deadline` is passed, call `wake_up_process` to wake up the task.

14. `deregister_process` function
- Delete the task from the task struct list, and delete `task->wakeup_timer`.
- If it's also the `cur_task`, set `cur_task` to NULL, and call 
`wake_up_process`.

15. `get_highest_priority_ready_task` function
- Loop through task struct list and find the `READY` task with the 
`highest priority`.

16. `__set_priority` function
- Set the `policy` & `priority` of the task.

17. `dispatch_thread_fn` function
- Check if the kthread should stop by calling `kthread_should_stop`.
- Get the `READY` task with the `highest priority`.
- If NO `READY` task, lower the priority of the `cur_task`.
- Else `cur_task` is preempted by `READY` task.
- Call `wake_up_process` on that task.

18. exit
- Remove `proc_entry` which is `/proc/mp1/status` file.
- Remove `proc_dir` which is `/proc/mp1` directory.
- Destroy 2 `mutex`.
- Stop kernel thread `dispatch_thread`.
- Delete & free the memory of the linked list with `list_for_each_entry_safe` 
macro.
- Free kernel memory cache.
- Destroy `struct kem_cache -rms_task_struct_cache`.

### userapp.h & userapp.c

1. `register_process` function
- Open `/proc/mp1/status` file and write `R, <pid>, <period>, <computation_time>`.

2. `process_exist` function
- Open `/proc/mp1/status` file and check if `pid` exists.

3. `yield_process` function
- Open `/proc/mp1/status` file and write `Y, <pid>`.

4. `deregister_process` function
- Open `/proc/mp1/status` file and write `D, <pid>`.

5. `main` function
- Loop `do_job` function with `freq` to simulate periodic task.
