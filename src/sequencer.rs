use std::ffi::CString;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_settings_t {
    // ... fields of the struct ...
}

#[allow(improper_ctypes)]
// # [ link(name = "fluid")]
extern "C" {
    fn new_fluid_settings() -> *mut fluid_settings_t;
    fn fluid_settings_setint(
        settings: *mut fluid_settings_t,
	name: *const i8,
	val: i32) -> i32;
}

fn createsynth() {
    println!("createsynth");
    unsafe {
        let settings_ptr = new_fluid_settings();
        // let settings = &mut *settings_ptr;
	let mut ret;
	let mut key;
	key =
	    CString::new("synth.reverb.active").expect("CString::new failed");
	ret  = fluid_settings_setint(settings_ptr, key.as_ptr(), 0);
	println!("setting reverb: ret={}", ret);
	key =
	    CString::new("synth.chorus.active").expect("CString::new failed");
	ret  = fluid_settings_setint(settings_ptr, key.as_ptr(), 0);
	println!("setting chorus: ret={}", ret);
    }
}

pub fn sequencer() {
    println!("sequencer");
    createsynth();
}


