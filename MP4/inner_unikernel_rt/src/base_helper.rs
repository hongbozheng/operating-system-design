use crate::stub;

// Helper definitions

pub(crate) fn bpf_get_current_pid_tgid() -> u64 {
    let ptr = stub::STUB_BPF_GET_CURRENT_PID_TGID as *const ();
    let code: extern "C" fn() -> u64 = unsafe { core::mem::transmute(ptr) };
    code()
}

pub(crate) fn bpf_trace_printk(fmt: &str, arg1: u64, arg2: u64, arg3: u64) -> i32 {
    let ptr = stub::STUB_BPF_TRACE_PRINTK_IU as *const ();
    let code: extern "C" fn(*const u8, u32, u64, u64, u64) -> i32 =
        unsafe { core::mem::transmute(ptr) };

    code(fmt.as_ptr(), fmt.len() as u32, arg1, arg2, arg3)
}

/// Wrapper function around an external function with the C ABI, used to read data from kernel memory.
///
/// ## Arguments
///
/// * `dst` - A mutable reference to a type `T` where the read data will be stored.
/// * `unsafe_ptr` - A pointer to some data in kernel memory to be read.
///
/// ## Returns
///
/// An `i64` value representing the result of calling the external function with the given arguments.
pub(crate) fn bpf_probe_read_kernel<T>(dst: &mut T, unsafe_ptr: *const ()) -> i64 {
    // TODO: Make it do something useful
    let ptr = stub::STUB_BPF_PROBE_READ_KERNEL as *const ();
    let code: extern "C" fn(*mut (), u32, *const ()) -> i64 = unsafe { core::mem::transmute(ptr) };
    let size = core::mem::size_of::<T>();
    code(dst as *mut T as *mut (), size as u32, unsafe_ptr)
}

// This macro is used to export the helpers to program object defintions
// You can check tracepoint/tp_impl.rs and kprobe/kprobe_impl.rs if you are
// interested in how they work
macro_rules! base_helper_defs {
    () => {
        pub fn bpf_get_current_pid_tgid(&self) -> u64 {
            crate::base_helper::bpf_get_current_pid_tgid()
        }

        pub fn bpf_trace_printk(&self, fmt: &str, arg1: u64, arg2: u64, arg3: u64) -> i32 {
            crate::base_helper::bpf_trace_printk(fmt, arg1, arg2, arg3)
        }

        pub fn bpf_probe_read_kernel<T>(&self, dst: &mut T, unsafe_ptr: *const ()) -> i64 {
            crate::base_helper::bpf_probe_read_kernel::<T>(dst, unsafe_ptr)
        }
    };
}

pub(crate) use base_helper_defs;