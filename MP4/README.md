# Rust-based inner-unikernels

**Note: You need to be on the x86-64 architecture in order to work on this
MP. We assume the x86-64 architecture and ABI in this writeup.**

**Please make sure you read through this document at least once before
starting.**

## Introduction (taken from our abstract)
Safe kernel extensions, in the form of eBPF in Linux, are enabling new
functionality from observability and debugging superpowers to
high-performance, close-to-the-hardware packet processing. The extension
code is verified to be safe and executed at any kernel hook points. Despite
its safety guarantees, the price of kernel verification of eBPF programs is
high. The restricted expressiveness of eBPF on loops or complex logic often
leads to splitting the program into small pieces, or limitation in
functionality when logic cannot be expressed. On the other hand, bypassing
the verifier means giving up safety and security. This prompts us to
rethink kernel extensions — how to offer the desired programmability
without giving up safety?

This research project makes the observation that the infrastructure around
eBPF is useful and the needed safety can still be guaranteed from a safe
language like Rust without the verifier. We design and implement a new
kernel extension abstraction: Rust-based inner unikernels, which are
standalone safe Rust programs that run in the place of verified eBPF
programs. The use of Rust achieves both Turing-Completeness and
runtime-safety. Our Rust layer provides access to the eBPF helper function
interface, a signature scheme ensures safety, and kernel support ensures
timely termination. We implement several state of the art eBPF programs
with comparable performance to JITed eBPF programs, demonstrating there is
no need to split programs or limit their processing data size.

## Problem Description
Your task is to implement an equivalent inner-unikernel program in Rust for
an existing eBPF program. It is conceptually very straightforward. However,
the real challenge is in hacking things out and understanding how
inner-unikernels work. The task will evaluate whether you have the
technical skills to quickly bootstrap yourself with Linux kernel
programming, Rust programming, and other important stuff (e.g. ELF).

You will write an inner-unikernel equivalent of the sample eBPF program
[`tracex5`](https://elixir.bootlin.com/linux/v5.15.63/source/samples/bpf/tracex5_kern.c).
The program attaches a kprobe eBPF program on the `__seccomp_filter`
function in the kernel. The `__seccomp_filter` function is part of the
Seccomp syscall filtering framework inside the Linux kernel. Seccomp
implements a system call policy to regulate the system call behavior of a
user space application. The function has the following signature:
```c
static int __seccomp_filter(int this_syscall, const struct seccomp_data *sd,
			    const bool recheck_after_trace)
```
where `this_syscall` is the system call number of the incoming syscall and
`sd` holds the system call related information (args, instruction pointer,
etc).

The way the BPF kprobe program works is that it is triggered every time the
`__seccomp_filter` function is entered. The program takes in an argument of
`struct pt_regs *` which containts the register states of the task when it
enters the function. The `tracex5` program first checks the system call
number and then "tail calls" into other programs that handles specific
system calls (e.g. `bpf_func_SYS__NR_write`, it's easier to think of it as
a function call).

`tracex5` uses some of the BPF helper functions to perform certain tasks,
e.g. `bpf_probe_read_kernel` reads arbitrary kernel memory safely and
`bpf_trace_printk` writes messages to the trace buffer. These helper
functions can be think of as the syscalls for eBPF programs. Our
inner-unikernel programs would also call the helper functions, therefore,
you will need to implement the missing helper functions on the Rust side.
The helper functions are documented in
[`include/uapi/linux/bpf.h`](https://elixir.bootlin.com/linux/v5.15.63/source/include/uapi/linux/bpf.h).

## Implementation Overview
We recommend approaching this MP in the following steps:

### Install the dependencies:
The following tools/libraries are required for this MP. Older versions are
not guaranteed to (or guaranteed not to) work.
- binutils version 2.38 or higher [homepage](https://www.gnu.org/software/binutils/)
- clang+LLVM toolchain version 14 or higher [homepage](https://llvm.org)
- gcc with c++17 support [homepage](https://gcc.gnu.org/)
- elfutils [homepage](https://sourceware.org/elfutils/)
- pahole [homepage](https://git.kernel.org/cgit/devel/pahole/pahole.git/)
- python toml package [homepage](https://github.com/uiri/toml)
- cmake [homepage](https://cmake.org/)
- ninja [homepage](https://ninja-build.org/)

Example configuration from Jinghao as a reference:
- binutils 2.39
- clang+LLVM 14.0.6
- gcc 12.2.1
- elfutils 0.188
- pahole 1.23
- python toml 0.10.2
- cmake 3.24.3
- ninja 1.11.1

### Setup the environment and the sample program
This project requires our custom rust compiler and Linux kernel. First you
need to build and install the new kernel:
```bash
git clone git@github.com:djwillia/linux.git /your/kernel/path
cd /your/kernel/path
git checkout inner_unikernels
cp /your/mp4/path/.config .config
# Then build your kernel following instructions from MP0
```

Once you have setup the kernel properly, you can move on to build the
needed libraries:
```bash
cd /your/kernel/path/tools/lib/bpf
make -j`nproc`
cd /your/mp4/path/
ln -s `realpath /your/kernel/path` linux # needed by later compilation
cd /your/mp4/path/libiu
make -j`nproc`
```

Now is time to build our rust compiler, you can follow the following steps:
```bash
git clone git@github.com:xlab-uiuc/rust.git /your/rust/path
mkdir /your/rust/path/../dist # Note: this relative path in important!
cd /your/rust/path
mv inner-unikernels-config.toml config.toml
./x.py build && ./x.py install # start the bootstrapping process
cd /your/mp4/path/
ln -s `realpath /your/rust/path/../dist` rust-dist # needed by later compilation
```
Note: This step could take more time than kernel compilation.

Then install [rust-bindgen](https://github.com/rust-lang/rust-bindgen) with
the following command:
```bash
PATH=`realpath /your/rust/path/../dist/bin`:$PATH cargo install bindgen-cli
```

Now you should have everything for building the sample `hello` program:
```bash
cd /your/rust/path/sample/hello
make
```

To run it, you need to be on the kernel you have just built (in case you
are not on that kernel), you can directly invoke
```bash
./loader
```
and you should see some similar output as the following:
```console
<...>-245     [002] d...1    18.417331: bpf_trace_printk: Rust triggered from PID 245.
```

We recommend you to take a look at both the hello program and the loader
file to get familiar with the task you are going to work on.

### Try out the original program
You can build it with the following command:
```bash
cd /your/kernel/path/samples/bpf
make -j`nproc`
```
Then, on the kernel you have just built, do:
```bash
cd /your/kernel/path/samples/bpf
./tracex5
```
You should make sure you understand what this original BPF program is
doing, as you are going to implement the same logic in Rust.

### Understand the repo structure
The repo has the following directories
- `libiu`: the equivalent of `libbpf` for inner-uniknernel programs. You
  should not change the file
- `inner-unikernel-rt`: the runtime crate for inner-uniknernel programs, it
  contains the program type and helper function definitions. You will need
  to add one more helper function to `base_helper.rs` but should not change
  any other files
- `samples/hello`: the `hello` example we have just seen
- `samples/tracex5`: the directory of the program you need to implement.
  Specifically, the inner-unikernel program code should go into
  `src/main.rs` and the loader code should go into `tracex5.c`. Again, you
  should not modify any other files.


### Make a minimal program
We suggest you to first make a small working inner-unikernel program. One
way to achieve this is to create the function and object similar to
`iu_prog1_fn` and `PROG` in the `hello` example. The function for now can
just do a `bpf_trace_printk` so that you will see the printed message when
your program is triggered. One caveate is that the section name is
important and decides the program type and attachment point. You should use
the same section name as in the original
[`tracex5`](https://elixir.bootlin.com/linux/v5.15.63/source/samples/bpf/tracex5_kern.c).

In order to make the program working, you also need to implement the loader
for your program. Again the loader for `hello` would be a good resource but
note that `hello` is not a kprobe program (rather, it is a tracepoint
program) and has a different attachment point. In order to attach the
program to the correct place, your loader should look similar to the
original
[loader](https://elixir.bootlin.com/linux/v5.15.63/source/samples/bpf/tracex5_user.c)
in the kernel sample.

One hint for implementing the loader is that `iu_object__open` should be
used in place of `bpf_object__open_file` for inner-unikernel programs.
Moreover, the `bpf_object__load` is no longer needed because
`iu_object__open` already takes care of it.

### Add the missing helper function
The `bpf_probe_read_kernel` is left unimplemented for you. You should
fill in the implementation and can use the existing helper functions in the
`inner_unikernel_rt` crate as reference. Do not modify other part of the
crate except the provided empty function body.

The helper stubs can be found under
`samples/tracex5/target/debug/build/inner_unikernel_rt-<some-randome-hash>/out`.

### Implement the actual logic
Once your program is attached and executed correctly, you can move on the
add the actual logic. The original program uses BPF tail calls to jump to
other BPF programs (functions). This is partly because BPF programs cannot make
arbitrary out-of-line function calls apart from helpers (in this sample is
more of an example of tail call usage). For us, Rust-based inner-unikernel
programs can call other functions even if they are not marked as inline and
we do not need the tail call functionality. This means we will not do tail
calls and do not need to define the `progs` map.

What you need to do for the program is to define 3 functions (with the same
logic as the original BPF program) for the `read`, `write`, and `mmap`
system calls (you can ignore `mmap2`) and call these functions from your
program function (i.e. `iu_prog1_fn` as in `hello`) based on the system
call number.

You will need to interact with some of the kernel bindings generated in the
`inner_unikernel_rt` crate. These are under the `inner_unikernel_rt::linux`
namespace. For example, for the `struct pt_regs` defined in `linux/ptrace.h`,
the binding in Rust is `inner_unikernel_rt::linux::ptrace::pt_regs`, which
shares exactly the same layout as the uapi C struct and is safe to be sent
through the FFI interface. The generated files can also be found under
`samples/tracex5/target/debug/build/inner_unikernel_rt-<some-randome-hash>/out`,
the same as the helper stubs.

After this step you should get similar output as the original program.

## Other Requirements
- Do not change any other files except the files mentioned above.
- Your Rust code should not have any `unsafe` block in the inner-unikernel
  program, but can have `unsafe` blocks in the `inner_unikernel_rt` crate.
- You should not use any other extern crates (i.e. Rust packages) other
  than the provided `inner_unikernel_rt` and the basic `rlibc`.
- You cannot use the Rust `std` library because it is not available in
  standalong mode, but the `core` library remains largely available.

## Resources
We recommend you to get your hands dirty directly and check these resources
on demand. In fact, we didn’t know Rust well when we started this project –
you can always learn a language by writing the code.
- eBPF: [ebpf.io](https://ebpf.io/) and
  [cilium.io](https://docs.cilium.io/en/stable/bpf/) both are good places
  to start. You can also find the official kernel documentation
  [here](https://elixir.bootlin.com/linux/v5.15.63/source/Documentation/bpf)
  along with the source code. In particular, try answering:
    - What is eBPF?
    - What are some example use cases of eBPF?
    - How are eBPF programs loaded to the kernel?
    - How are the execution of eBPF programs triggered?
    - What are eBPF helpers?
- Rust: If you are not familiar with the Rust program language, we have some resources for you:
    - [The Rust book](https://doc.rust-lang.org/book/) (Probably the most comprehensive guide on Rust programming)
    - [Library API reference](https://doc.rust-lang.org/std/index.html) (for searching API specifications)
    - [The Rust playground](https://play.rust-lang.org) (for trying out programs)

## Design Documentation
1. Follow the linux [tracex5_user.c](https://elixir.bootlin.com/linux/v5.15.79/source/samples/bpf/tracex5_kern.c) and implement `tracex5.c`.
- Open & load object.
- Find the program in a BPF object.
- Attach a BPF program to a socket in the kernel.

2. Follow the linux [tracex5_kern.c](https://elixir.bootlin.com/linux/v5.15.79/source/samples/bpf/tracex5_kern.c)
Implement function `iu_prog1`
- Determine the system call number and call the appropriate function to handle 
the system call.
- If the system call number is not handled by any of the specific functions, 
  the function will print a message if the system call number is within a 
  certain range (i.e. between `__NR_getuid` and `__NR_getsid` inclusive).

Implement function `SYS__NR_write`
- A function that is called when a `SYS_write` system call is executed
- The function uses a `kprobe` and `pt_regs` object to read arguments from the 
  system call and print a message if the `size` argument is 512.

Implement function `SYS__NR_read`
- a function that is called when a `SYS_read` system call is executed
- the function uses a `kprobe` and `pt_regs` object to read arguments from the 
  system call and print a message if the `size` argument is between 128 and 1024 
  (inclusive).

Implement function `SYS__NR_mmap`
- A function that is called when a `SYS_mmap` system call is executed.
- The function uses a `kprobe` object to print a message indicating that the 
  `SYS_mmap` system call has been executed.

Follow `main.rs` in the `hello` program
- Define a BPF program in Linux kernel.

3. Follow the [bpf_probe_read_kernel](https://elixir.bootlin.com/linux/v5.15.79/source/kernel/bpf/core.c#L1369) prototype and the functions in 
   `base_helper.rs`
- Implement function `bpf_probe_read_kernel`.
