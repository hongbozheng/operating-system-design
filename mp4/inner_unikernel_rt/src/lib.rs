#![no_std]

pub mod linux;
pub mod prog_type;
pub mod tracepoint;
pub mod kprobe;

mod base_helper;
mod stub;

use crate::prog_type::iu_prog;
use core::panic::PanicInfo;

#[no_mangle]
fn __iu_entry_tracepoint(prog: &tracepoint::tracepoint, ctx: *const ()) -> u32 {
    prog.prog_run(ctx)
}

#[no_mangle]
fn __iu_entry_kprobe(prog: &kprobe::kprobe, ctx: *const ()) -> u32 {
    prog.prog_run(ctx)
}

// This function is called on panic.
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}