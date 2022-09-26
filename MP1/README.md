# MP1 - Introduction to Linux Kernel Programming
##### Instructor: Tianyin Xu
Deadline --- Tuesday, September, 27th, 11:59PM

Create a Linux Kernel Module (LKM) that measures the Userspace CPU Time of processes registered within the kernel module

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

## Test Linux Kernel Module
#### 1. Test with 1 userapp
Run the following command
```
test_kernel_module_1.sh
```

#### 2. Test with 2 userapp
Run the following command
```
test_kernel_module_2.sh
```

## Design

##### 1. Initialize `struct proc_dir_entry`
* `proc_dir` for directory
* `proc_entry` for status file

##### 2. Create process directory and file
* create `/proc/mp1` directory
* create `/proc/mp1/status` file

##### 3. Initialize other module
* `spinlock` write protection
* `struct timer_list` check cpu_time after timer expires
* `struct workqueue_struct` schedule actions to run in process context
* `struct work_struct` schedule a task to run at a later time

##### 4. Create `struct proc_ops`
* `proc_read` process read from file
* `proc_write`  process write to file
* `proc_release` process closes the file

##### 5. `proc_read` function
* Check if the user already read part of the file
* Continue from the file left off position if the file is already read
* Otherwise read from the beginning of the file and then update the file offset `offl`
* Create struct `status_buf` to store the status file buffer and size

##### 6. `proc_write` function
* In `userapp.c` open `/proc/mp1/status` file and write the `process ID (PID)` in the file
* In `mp1.c`, use `sscanf` to write `PID` into `proc_struct->pid`

##### 7. `proc_release` function
* free `file->private_data` when the process closes the file

##### 8. Timer & Interrupt
* For each 5 seconds, the timer `callback` function will be triggered
* When `callback` function is called, it will add `work` which is the function `update_cpu_time` into the queue
* Then it sets the timer expiration time to 5 seconds in the future

##### 9. workqueue & work
* `workqueue` allow kernel functions to be activated and later executed by special kernel threads called `worker` threads
* `work` init `work` to be the function `update_cpu_time`
* `update_cpu_time` function will traverse the linked list, and use the function `get_cpu_time` in `mp1_given.h` file to update the cpu_time for each process
* Everytime the timer expires, the `work` is enqueued into `workqueue`

##### 10. exit
* remove `proc_entry` which is `/proc/mp1/status` file
* remove `proc_dir` which is `/proc/mp1` directory
* delete & free `timer`
* delete & free the memory of the linked list with `list_for_each_entry_safe` macro
* ensure that any scheduled work has run to completion with `flush_workqueue`, and safely destroy a `workqueue`
* free the memory of `struct work_struct`

## Developers
* Hongbo Zheng [NetID: hongboz2]
