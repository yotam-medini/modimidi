use std::fmt;
use std::ffi::CString;
use crate::cfluid;
pub struct SequencerControl {
    settings_ptr: *mut cfluid::fluid_settings_t,
    pub synth_ptr: *mut cfluid::fluid_synth_t,
    audio_driver_ptr: *mut cfluid::fluid_audio_driver_t,
    pub sequencer_ptr: *mut cfluid::fluid_sequencer_t,
    pub synth_seq_id: i16,
    pub sfont_id: i32,
    pub periodic_seq_id: i16,
    pub final_seq_id: i16,
    pub progress_seq_id: i16,
    pub batch_duration_ms: u32,
    pub initial_delay_ms: u32,
    pub add_ms: u32,
}

impl fmt::Display for SequencerControl {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, concat!(
           "SequencerControl(settings={:?}, syn={:?}, a={:?}, seq={:?}, seq_id={}, ",
           "final={}, my={}, ",
           "batch_duration_ms={}, initial_delay_ms={}, "),
           self.settings_ptr,
           self.synth_ptr, self.audio_driver_ptr, self.sequencer_ptr,
           self.synth_seq_id, self.periodic_seq_id, self.final_seq_id,
           self.batch_duration_ms, self.initial_delay_ms)
    }
}

fn create_synth(seq_ctl: &mut SequencerControl, sound_font_path: &String) {
    unsafe {
        seq_ctl.settings_ptr = cfluid::new_fluid_settings();
        let mut ret;
        let mut key;
        key =
            CString::new("synth.reverb.active").expect("CString::new failed");
        ret = cfluid::fluid_settings_setint(seq_ctl.settings_ptr, key.as_ptr(), 0);
        if ret != cfluid::FLUID_OK {
            eprintln!("setting reverb: ret={}", ret);
        }
        key = CString::new("synth.chorus.active").expect("CString::new failed");
        ret  = cfluid::fluid_settings_setint(seq_ctl.settings_ptr, key.as_ptr(), 0);
        if ret != cfluid::FLUID_OK {
            eprintln!("setting chorus: ret={}", ret);
        }
        let _synth = cfluid::new_fluid_synth(seq_ctl.settings_ptr);
        seq_ctl.synth_ptr = cfluid::new_fluid_synth(seq_ctl.settings_ptr);
        let sf_path = sound_font_path.to_owned(); 
        let c_str_sf_path = CString::new(sf_path).unwrap();
        seq_ctl.sfont_id = cfluid::fluid_synth_sfload(
            seq_ctl.synth_ptr, c_str_sf_path.as_ptr(), 1);
        seq_ctl.audio_driver_ptr =
            cfluid::new_fluid_audio_driver(seq_ctl.settings_ptr, seq_ctl.synth_ptr);
        seq_ctl.sequencer_ptr = cfluid::new_fluid_sequencer2(0);

        // register synth as first destination
        seq_ctl.synth_seq_id = cfluid::fluid_sequencer_register_fluidsynth(
            seq_ctl.sequencer_ptr, seq_ctl.synth_ptr);

        println!("sequencer time_scale={}",
            cfluid::fluid_sequencer_get_time_scale(seq_ctl.sequencer_ptr));
    }
}

fn load_sound_font(synth_ptr: *mut cfluid::fluid_synth_t) {
    let path = CString::new("/usr/share/sounds/sf2/FluidR3_GM.sf2").expect(
        "CString::new failed");
    unsafe {
        let _ = cfluid::fluid_synth_sfload(synth_ptr, path.as_ptr(), 1);
    }
}

pub fn create_sequencer(
    sound_font_path: &String,
    batch_duration_ms: u32,
    initial_delay_ms: u32) -> SequencerControl {
    let mut sequencer = SequencerControl {
        settings_ptr: std::ptr::null_mut(),
        synth_ptr: std::ptr::null_mut(),
        audio_driver_ptr: std::ptr::null_mut(),
        sequencer_ptr: std::ptr::null_mut(),
        synth_seq_id: 0,
        sfont_id: -1,
        periodic_seq_id: 0,
        final_seq_id: 0,
        progress_seq_id: 0,
        batch_duration_ms: batch_duration_ms,
        initial_delay_ms: initial_delay_ms,
        add_ms: 0, // to be set later
    };
    create_synth(&mut sequencer, sound_font_path);
    load_sound_font(sequencer.synth_ptr);
    sequencer
}

pub fn destroy_sequencer(sequencer: &mut SequencerControl) {
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
