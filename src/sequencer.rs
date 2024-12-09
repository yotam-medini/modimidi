use std::fmt;
use std::ffi::CString;
use crate::cfluid;

pub struct Sequencer {
    settings_ptr: *mut cfluid::fluid_settings_t,
    pub synth_ptr: *mut cfluid::fluid_synth_t,
    audio_driver_ptr: *mut cfluid::fluid_audio_driver_t,
    pub sequencer_ptr: *mut cfluid::fluid_sequencer_t,
    pub synth_seq_id: i16,
    pub sfont_id: i32,
    pub my_seq_id: i16,
    pub now: u32,
}

impl fmt::Display for Sequencer {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, concat!(
           "Structure(settings={:?}, syn={:?}, a={:?}, seq={:?}, seq_id={}, my={}, ",
           "now={}"),
           self.settings_ptr,
           self.synth_ptr, self.audio_driver_ptr, self.sequencer_ptr,
           self.synth_seq_id, self.my_seq_id,
           self.now)
    }
}

#[cfg(feature = "obsolete_seq_callback")]
extern "C" fn seq_callback(
    time: u32,
    event: *mut cfluid::fluid_event_t,
    seq: *mut cfluid::fluid_sequencer_t, 
    data: *mut c_void) {
    unsafe {
        let the_sequencer = &mut *(data as *mut Sequencer);
        println!("seq_callback: time={}, event={:?}, eq={:?}, the_sequencer={}",
            time, event, seq, the_sequencer);
    }
}

fn create_synth(sequencer: &mut Sequencer, sound_font_path: &String) {
    println!("create_synth");
    unsafe {
        sequencer.settings_ptr = cfluid::new_fluid_settings();
	let mut ret;
	let mut key;
	key =
	    CString::new("synth.reverb.active").expect("CString::new failed");
	ret  = cfluid::fluid_settings_setint(sequencer.settings_ptr, key.as_ptr(), 0);
	println!("setting reverb: ret={}", ret);
	key =
	    CString::new("synth.chorus.active").expect("CString::new failed");
	ret  = cfluid::fluid_settings_setint(sequencer.settings_ptr, key.as_ptr(), 0);
	println!("setting chorus: ret={}", ret);
	let _synth = cfluid::new_fluid_synth(sequencer.settings_ptr);
	sequencer.synth_ptr = cfluid::new_fluid_synth(sequencer.settings_ptr);
        let sf_path = sound_font_path.to_owned(); 
        let c_str_sf_path = CString::new(sf_path).unwrap();
        sequencer.sfont_id = cfluid::fluid_synth_sfload(
            sequencer.synth_ptr, c_str_sf_path.as_ptr(), 1);
        sequencer.audio_driver_ptr =
            cfluid::new_fluid_audio_driver(sequencer.settings_ptr, sequencer.synth_ptr);
        sequencer.sequencer_ptr = cfluid::new_fluid_sequencer2(0);

        // register synth as first destination
        sequencer.synth_seq_id = cfluid::fluid_sequencer_register_fluidsynth(
            sequencer.sequencer_ptr, sequencer.synth_ptr);

        println!("sequencer time_scale={}",
            cfluid::fluid_sequencer_get_time_scale(sequencer.sequencer_ptr));
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

pub fn create_sequencer(sound_font_path: &String) -> Sequencer {
    println!("create_sequencer({})", sound_font_path);
    let mut sequencer = Sequencer {
        settings_ptr: std::ptr::null_mut(),
        synth_ptr: std::ptr::null_mut(),
        audio_driver_ptr: std::ptr::null_mut(),
        sequencer_ptr: std::ptr::null_mut(),
        synth_seq_id: 0,
        sfont_id: -1,
        my_seq_id: 0,
        now: 0,
    };
    create_synth(&mut sequencer, sound_font_path);
    println!(
        concat!(
            "sequencer: synth_ptr={:?}, audio_driver_ptr={:?}, ",
            "sequencer_ptr={:?}, ",
            "synth_seq_id={}, my_seq_id={}, ",
            "now={}"),
        sequencer.synth_ptr, sequencer.audio_driver_ptr,
        sequencer.sequencer_ptr,
        sequencer.synth_seq_id, sequencer.my_seq_id,
        sequencer.now);
    load_sound_font(sequencer.synth_ptr);
    sequencer
}

pub fn destroy_sequencer(sequencer: &mut Sequencer) {
    println!("destroy_synth");
    unsafe {
        cfluid::delete_fluid_sequencer(sequencer.sequencer_ptr);
        sequencer.sequencer_ptr = std::ptr::null_mut();
        cfluid::delete_fluid_audio_driver(sequencer.audio_driver_ptr);
        sequencer.audio_driver_ptr = std::ptr::null_mut();
        cfluid::delete_fluid_synth(sequencer.synth_ptr);
        sequencer.synth_ptr = std::ptr::null_mut();
        cfluid::delete_fluid_settings(sequencer.settings_ptr);    
        sequencer.settings_ptr = std::ptr::null_mut();
    }
}
