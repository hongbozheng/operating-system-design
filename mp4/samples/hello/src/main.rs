#![no_std] // `std::*` is not available for us
#![no_main] // No need for a `main` because this is not a userspace program

extern crate inner_unikernel_rt; // runtime library
extern crate rlibc; // in case `memset`, `memcpy`, and `memmove` are needed

use inner_unikernel_rt::tracepoint::*;

// This is the actual code for the inner-unikernel program, as a function
//
// Hint: Helper functions are implemented as methods for the program object
fn iu_prog1_fn(obj: &tracepoint, ctx: &tp_ctx) -> u32 {
    // Get the current PID
    let pid = (obj.bpf_get_current_pid_tgid() & 0xFFFFFFFF) as u32;
    // Print the "Rust triggered from PID <PID>.\n" message to the trace buffer
    // i.e. the /sys/kernel/debug/tracing/trace file
    obj.bpf_trace_printk("Rust triggered from PID %u.\n", pid as u64, 0, 0);
    return 0;
}

// This is the program object that defines the program
// i.e. making it recognizable by the compiler and the loader library.
// The compiler will generate a function that calls `__iu_entry_tracepoint`
// from the runtime library with this object as an argument. This generated
// function is called every time the program gets triggered in the kernel.
//
// For the `new` method: this is implemented by all program types, right now
// the runtime crate has tracepoint and kprobe. You can check the
// src/tracepoint/tp_impl.rs and src/kprobe/kprobe_impl.rs files in the runtime
// library for the actual signature. For you convenience, we also listed the
// arguments for `tracepoint::new` below (and `kprobe::new` has a similar
// signature):
//
// arg1: function contains the actual code for the program. i.e. `iu_prog1_fn`
// arg2: the desired program name, used by the
//       `bpf_object__find_program_by_name` function in the loader program
// arg3: tracepoint specific argument, you don't need to worry about this for
//       your kprobe program
//
// `link_section` is equivalent to the `SEC` macro in BPF programs. It specifies
// the program type and attach point. In this case, it means the program is a
// tracepoint program and should be attached to the entry of the `dup` syscall.
#[link_section = "tracepoint/syscalls/sys_enter_dup"]
static PROG: tracepoint = tracepoint::new(iu_prog1_fn, "iu_prog1", tp_ctx::Void);