fn main() {
    println!("cargo:rustc-link-lib=fluidsynth"); // Link with libfluidsynth
    println!("cargo:rustc-link-search=/usr/lib/x86_64-linux-gnu"); // Add the library path
}
