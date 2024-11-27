use std::fmt;
use std::ffi::CString;
use std::os::raw::c_void;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_audio_driver_t {
    // ... fields of the struct ...
}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_event_t {
    // ... fields of the struct ...
}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_sequencer_t {
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

type FluidSeqId = i16; // fluid_seq_id_t

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
    fn fluid_sequencer_register_client(
        seq: *mut fluid_sequencer_t, 
        name: *const i8,
        callback: extern "C" fn(
            time: u32,
            event: *mut fluid_event_t,
            seq: *mut fluid_sequencer_t, 
            data: *mut c_void),
        data: *mut c_void) -> FluidSeqId;
    fn fluid_sequencer_register_fluidsynth(
       seq: *mut fluid_sequencer_t,
       synth: *mut fluid_synth_t) -> FluidSeqId;
    fn new_fluid_sequencer2(use_system_timer: i32) -> *mut fluid_sequencer_t;
    fn new_fluid_synth(settings: *mut fluid_settings_t) -> *mut fluid_synth_t;
}

struct Sequencer {
    synth_ptr: *mut fluid_synth_t,
    audio_driver_ptr: *mut fluid_audio_driver_t,
    sequencer_ptr: *mut fluid_sequencer_t,
    synth_seq_id: i16,
    my_seq_id: i16,
    now: u32,
    seq_duration: u32,
}

impl fmt::Display for Sequencer {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, concat!(
           "Structure(syn={:?}, a={:?}, seq={:?}, seq_id={}, my={}, ",
           "now={}, dur={}"),
           self.synth_ptr, self.audio_driver_ptr, self.sequencer_ptr,
           self.synth_seq_id, self.my_seq_id,
           self.now, self.seq_duration)
    }
}

extern "C" fn seq_callback(
    time: u32,
    event: *mut fluid_event_t,
    seq: *mut fluid_sequencer_t, 
    data: *mut c_void) {
    unsafe {
        let the_sequencer = &mut *(data as *mut Sequencer);
        println!("seq_callback: time={}, event={:?}, eq={:?}, the_sequencer={}",
            time, event, seq, the_sequencer);
    }
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
	sequencer.synth_ptr = new_fluid_synth(settings_ptr);
        sequencer.audio_driver_ptr =
            new_fluid_audio_driver(settings_ptr, sequencer.synth_ptr);
        sequencer.sequencer_ptr = new_fluid_sequencer2(0);

        // register synth as first destination
        sequencer.synth_seq_id = fluid_sequencer_register_fluidsynth(
            sequencer.sequencer_ptr, sequencer.synth_ptr);

        // register myself as second destination
	key = CString::new("me").expect("CString::new failed");
        sequencer.my_seq_id = fluid_sequencer_register_client(
            sequencer.sequencer_ptr, 
            key.as_ptr(),
            seq_callback, 
            std::ptr::null_mut());
        sequencer.seq_duration = 1000;
    }
}

pub fn sequencer() {
    println!("sequencer");
    let mut sequencer = Sequencer {
        synth_ptr: std::ptr::null_mut(),
        audio_driver_ptr: std::ptr::null_mut(),
        sequencer_ptr: std::ptr::null_mut(),
        synth_seq_id: 0,
        my_seq_id: 0,
        now: 0,
        seq_duration: 0,
    };
    create_synth(&mut sequencer);
    println!(
        concat!(
            "sequencer: synth_ptr={:?}, audio_driver_ptr={:?}, ",
            "sequencer_ptr={:?}, ",
            "synth_seq_id={}, my_seq_id={}, ",
            "now={}, dur={}"),
        sequencer.synth_ptr, sequencer.audio_driver_ptr,
        sequencer.sequencer_ptr,
        sequencer.synth_seq_id, sequencer.my_seq_id,
        sequencer.now, sequencer.seq_duration);
}


