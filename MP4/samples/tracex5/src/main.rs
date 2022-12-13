#![no_std] // `std::*` is not available for us
#![no_main] // No need for a `main` because this is not a userspace program

extern crate inner_unikernel_rt; // runtime library
extern crate rlibc; // in case `memset`, `memcpy`, and `memmove` are needed

// Do not change above this line

// These imports should be enough for you
use inner_unikernel_rt::kprobe::*;
use inner_unikernel_rt::linux::ptrace::pt_regs;
use inner_unikernel_rt::linux::seccomp::seccomp_data;
use inner_unikernel_rt::linux::unistd::*;

// Helpers for you to use in the place of PT_REGS_PARM1 and PT_REGS_PARM2
// macros in C
#[inline(always)]
fn PT_REGS_PARM1(regs: &pt_regs) -> u64 {
    regs.rdi
}

#[inline(always)]
fn PT_REGS_PARM2(regs: &pt_regs) -> u64 {
    regs.rsi
}

// TODO: Write your inner-unikernel program here, you should refer to the
// original program:
// https://elixir.bootlin.com/linux/v5.15.79/source/samples/bpf/tracex5_kern.c
// You should also refer to samples/hello on how to make you function a
// inner-unikernel-program that is recognized by the compiler and loader library

/// A function that is called when a system call is executed.
///
/// This function uses a `kprobe` and `pt_regs` object to determine the system call number and call
/// the appropriate function to handle the system call. If the system call number is not handled by
/// any of the specific functions, the function will print a message if the system call number is
/// within a certain range (i.e. between `__NR_getuid` and `__NR_getsid` inclusive).
///
/// ## Arguments
///
/// * `obj` - A `kprobe` object that contains information about the probe point.
/// * `ctx` - A `pt_regs` object that contains the register values at the time of the probe.
///
/// ## Returns
///
/// This function returns a `u32` value representing the result of the system call. In this case, the
/// value is always 0.
fn iu_prog1(obj: &kprobe, ctx: &pt_regs) -> u32 {
    let sc_nr = PT_REGS_PARM1(ctx) as u32;
    match sc_nr {
        __NR_write => { SYS__NR_write(obj, ctx); },
        __NR_read => { SYS__NR_read(obj, ctx); },
        __NR_mmap => { SYS__NR_mmap(obj, ctx); },
        _ => {
            if sc_nr >= __NR_getuid && sc_nr <= __NR_getsid {
                obj.bpf_trace_printk("syscall=%d (one of get/set uid/pid/gid)\n", sc_nr as u64, 0, 0);
            }
        },
    }
    return 0;
}

/// A function that is called when a `SYS_write` system call is executed.
///
/// This function uses a `kprobe` and `pt_regs` object to read arguments from the system call and print
/// a message if the `size` argument is 512.
///
/// ## Arguments
///
/// * `obj` - A `kprobe` object that contains information about the probe point.
/// * `ctx` - A `pt_regs` object that contains the register values at the time of the probe.
///
/// ## Returns
///
/// This function returns a `u32` value representing the result of the system call. In this case, the
/// value is always 0.
fn SYS__NR_write(obj: &kprobe, ctx: &pt_regs) -> u32 {
    let mut sd = seccomp_data {
        nr: 0,
        arch: 0,
        instruction_pointer: 0,
        args: [0; 6usize],
    };
    obj.bpf_probe_read_kernel(&mut sd, PT_REGS_PARM2(ctx) as *const ());
    if (sd.args[2] == 512) {
        obj.bpf_trace_printk("write(fd=%d, buf=%p, size=%d)\n", sd.args[0], sd.args[1], sd.args[2]);
    }
    return 0;
}

/// A function that is called when a `SYS_read` system call is executed.
///
/// This function uses a `kprobe` and `pt_regs` object to read arguments from the system call and print
/// a message if the `size` argument is between 128 and 1024 (inclusive).
///
/// ## Arguments
///
/// * `obj` - A `kprobe` object that contains information about the probe point.
/// * `ctx` - A `pt_regs` object that contains the register values at the time of the probe.
///
/// ## Returns
///
/// This function returns a `u32` value representing the result of the system call. In this case, the
/// value is always 0.
fn SYS__NR_read(obj: &kprobe, ctx: &pt_regs) -> u32 {
    let mut sd = seccomp_data {
        nr: 0,
        arch: 0,
        instruction_pointer: 0,
        args: [0; 6usize],
    };
    obj.bpf_probe_read_kernel(&mut sd, PT_REGS_PARM2(ctx) as *const ());
    if (sd.args[2] > 128 && sd.args[2] <= 1024) {
        obj.bpf_trace_printk("read(fd=%d, buf=%p, size=%d)\n", sd.args[0], sd.args[1], sd.args[2]);
    }
    return 0;
}

/// A function that is called when a `SYS_mmap` system call is executed.
///
/// This function uses a `kprobe` object to print a message indicating that the `SYS_mmap` system call
/// has been executed.
///
/// ## Arguments
///
/// * `obj` - A `kprobe` object that contains information about the probe point.
/// * `ctx` - A `pt_regs` object that contains the register values at the time of the probe.
///
/// ## Returns
///
/// This function returns a `u32` value representing the result of the system call. In this case, the
/// value is always 0.
fn SYS__NR_mmap(obj: &kprobe, ctx: &pt_regs) -> u32 {
    obj.bpf_trace_printk("mmap\n", 0, 0, 0);
    return 0;
}

// The #[link_section] attribute specifies the section of the ELF file where the PROG variable
// should be placed. In this case, the variable will be placed in the "kprobe/__seccomp_filter"
// section of the ELF file.
#[link_section = "kprobe/__seccomp_filter"]
// define a BPF program in Linux kernel
static PROG: kprobe = kprobe::new(iu_prog1, "iu_prog1");