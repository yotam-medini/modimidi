use std::ffi::CString;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_audio_driver_t {
    // ... fields of the struct ...
}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_settings_t {
    // ... fields of the struct ...
}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_synth_t {
    // ... fields of the struct ...
}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_sequencer_t {
    // ... fields of the struct ...
}

#[allow(improper_ctypes)]
// # [ link(name = "fluid")]
extern "C" {
    fn fluid_settings_setint(
        settings: *mut fluid_settings_t,
	name: *const i8,
	val: i32) -> i32;
    fn new_fluid_settings() -> *mut fluid_settings_t;
    fn new_fluid_audio_driver(
        settings: *mut fluid_settings_t,
        synth: *mut fluid_synth_t) -> *mut fluid_audio_driver_t;
    fn new_fluid_synth(settings: *mut fluid_settings_t) -> *mut fluid_synth_t;
}

struct Sequencer {
    synth: *mut fluid_synth_t,
    a_driver: *mut fluid_audio_driver_t,
    synth_seq_id: i16,
    my_seq_id: i16,
    now: u32,
    seq_duration: u32,
}

fn create_synth(sequencer: &mut Sequencer) {
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
	let _synth = new_fluid_synth(settings_ptr);
	sequencer.synth = new_fluid_synth(settings_ptr);
        sequencer.a_driver =
            new_fluid_audio_driver(settings_ptr, sequencer.synth);
    }
}

pub fn sequencer() {
    println!("sequencer");
    let mut sequencer = Sequencer {
         synth: std::ptr::null_mut(),
         a_driver: std::ptr::null_mut(),
	 synth_seq_id: 0,
	 my_seq_id: 0,
	 now: 0,
	 seq_duration: 0,
    };
    create_synth(&mut sequencer);
    println!(
        concat!("sequencer: synth={:?}, synth_seq_id={}, my_seq_id={}, ",
             "now={}, dur={}"),
        sequencer.synth, sequencer.synth_seq_id, sequencer.my_seq_id,
        sequencer.now, sequencer.seq_duration);
}


