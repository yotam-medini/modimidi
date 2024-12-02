use std::{thread, time};
use std::fmt;
use std::os::raw::c_void;
use std::ffi::CString;
use crate::cfluid;
use crate::midi;

struct Sequencer {
    synth_ptr: *mut cfluid::fluid_synth_t,
    audio_driver_ptr: *mut cfluid::fluid_audio_driver_t,
    sequencer_ptr: *mut cfluid::fluid_sequencer_t,
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

fn schedule_next_sequence(sequencer: &mut Sequencer) {
    eprintln!("schedule_next_sequence - not yet");
}

extern "C" fn seq_callback(
    time: u32,
    event: *mut cfluid::fluid_event_t,
    seq: *mut cfluid::fluid_sequencer_t, 
    data: *mut c_void) {
    unsafe {
        let the_sequencer = &mut *(data as *mut Sequencer);
        println!("seq_callback: time={}, event={:?}, eq={:?}, the_sequencer={}",
            time, event, seq, the_sequencer);
        schedule_next_sequence(the_sequencer);
    }
}

fn create_synth(sequencer: &mut Sequencer) {
    println!("create_synth");
    unsafe {
        let settings_ptr = cfluid::new_fluid_settings();
        // let settings = &mut *settings_ptr;
	let mut ret;
	let mut key;
	key =
	    CString::new("synth.reverb.active").expect("CString::new failed");
	ret  = cfluid::fluid_settings_setint(settings_ptr, key.as_ptr(), 0);
	println!("setting reverb: ret={}", ret);
	key =
	    CString::new("synth.chorus.active").expect("CString::new failed");
	ret  = cfluid::fluid_settings_setint(settings_ptr, key.as_ptr(), 0);
	println!("setting chorus: ret={}", ret);
	let _synth = cfluid::new_fluid_synth(settings_ptr);
	sequencer.synth_ptr = cfluid::new_fluid_synth(settings_ptr);
        sequencer.audio_driver_ptr =
            cfluid::new_fluid_audio_driver(settings_ptr, sequencer.synth_ptr);
        sequencer.sequencer_ptr = cfluid::new_fluid_sequencer2(0);

        // register synth as first destination
        sequencer.synth_seq_id = cfluid::fluid_sequencer_register_fluidsynth(
            sequencer.sequencer_ptr, sequencer.synth_ptr);

        // register myself as second destination
	key = CString::new("me").expect("CString::new failed");
        sequencer.my_seq_id = cfluid::fluid_sequencer_register_client(
            sequencer.sequencer_ptr, 
            key.as_ptr(),
            seq_callback, 
            sequencer as *mut _ as *mut c_void);
        sequencer.seq_duration = 1000;
    }
}

fn destroy_synth(sequencer: &mut Sequencer) {
    println!("destroy_synth");
    unsafe {
        cfluid::delete_fluid_sequencer(sequencer.sequencer_ptr);
        sequencer.sequencer_ptr = std::ptr::null_mut();
        cfluid::delete_fluid_audio_driver(sequencer.audio_driver_ptr);
        sequencer.audio_driver_ptr = std::ptr::null_mut();
        cfluid::delete_fluid_synth(sequencer.synth_ptr);
        sequencer.synth_ptr = std::ptr::null_mut();
    }
}

fn load_sound_font(synth_ptr: *mut cfluid::fluid_synth_t) {
    let path = CString::new("/usr/share/sounds/sf2/FluidR3_GM.sf2").expect(
        "CString::new failed");
    unsafe {
        let fond_id = cfluid::fluid_synth_sfload(synth_ptr, path.as_ptr(), 1);
        println!("load_sound_font: fond_id={}", fond_id);
    }
}

fn send_note_on(sequencer: &mut Sequencer, chan: i32, key: i16, date: u32) {
    unsafe {
        let evt = cfluid::new_fluid_event();
        println!("evt={:?}", evt);
        cfluid::fluid_event_set_source(evt, -1);
        cfluid::fluid_event_set_dest(evt, sequencer.synth_seq_id);
        cfluid::fluid_event_noteon(evt, chan, key, 127);
        println!("send_note_on: date={}", date);
        let fluid_res = cfluid::fluid_sequencer_send_at(
            sequencer.sequencer_ptr, evt, date, 1);
        println!("send_note_on: fluid_res={}", fluid_res);
        cfluid::delete_fluid_event(evt);
    }
}

pub fn play(parsed_midi: &midi::Midi) {
    println!("play...");
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
            "play: sequencer: synth_ptr={:?}, audio_driver_ptr={:?}, ",
            "sequencer_ptr={:?}, ",
            "synth_seq_id={}, my_seq_id={}, ",
            "now={}, dur={}"),
        sequencer.synth_ptr, sequencer.audio_driver_ptr,
        sequencer.sequencer_ptr,
        sequencer.synth_seq_id, sequencer.my_seq_id,
        sequencer.now, sequencer.seq_duration);
    load_sound_font(sequencer.synth_ptr);
    unsafe {
        sequencer.now = cfluid::fluid_sequencer_get_tick(sequencer.sequencer_ptr);
        println!("play: tick={}", sequencer.now);
    }
    schedule_next_sequence(&mut sequencer);
    thread::sleep(time::Duration::from_millis(2000));
    unsafe {
        let tick = cfluid::fluid_sequencer_get_tick(sequencer.sequencer_ptr);
        println!("after sleep tick={}", tick);
        send_note_on(&mut sequencer, 0, 65, tick);
        thread::sleep(time::Duration::from_millis(300));
    }
    destroy_synth(&mut sequencer);
}
