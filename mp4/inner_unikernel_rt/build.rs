use std::env;
use std::process::Command;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let linux_dir = env::var("LINUX").unwrap();

    Command::new("python3")
        .arg("build.py")
        .arg(&format!("{}", linux_dir))
        .arg(&format!("{}", out_dir))
        .status()
        .unwrap();

    println!("cargo:rerun-if-changed=Cargo.toml");
}