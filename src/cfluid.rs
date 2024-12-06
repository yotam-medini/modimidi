use std::os::raw::c_void;

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_audio_driver_t {}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_event_t {}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_sequencer_t {}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_settings_t {}

#[repr(C)]
#[derive(Debug, Default, Copy, Clone)]
pub struct fluid_synth_t {}

type FluidSeqId = i16; // fluid_seq_id_t

#[allow(improper_ctypes)]
extern "C" {
    pub fn delete_fluid_audio_driver(driver: *mut fluid_audio_driver_t);
    pub fn delete_fluid_event(evt: *mut fluid_event_t);
    pub fn delete_fluid_sequencer(seq: *mut fluid_sequencer_t);
    pub fn delete_fluid_synth(synth: *mut fluid_synth_t);
    pub fn fluid_event_note(
        evt: *mut fluid_event_t,
        channel: i32,
        key: i16, 
        vel: i16,
        duration: u32);
    pub fn fluid_event_noteon(
        evt: *mut fluid_event_t,
        channel: i32,
        key: i16, 
        vel: i16);
    pub fn fluid_event_set_dest(evt: *mut fluid_event_t, dest: FluidSeqId);
    pub fn fluid_event_set_source(evt: *mut fluid_event_t, src: FluidSeqId);
    pub fn fluid_event_timer(evt: *mut fluid_event_t, data: *mut c_void);
    pub fn fluid_sequencer_get_tick(seq: *mut fluid_sequencer_t) -> u32;
    pub fn fluid_sequencer_register_client(
        seq: *mut fluid_sequencer_t, 
        name: *const i8,
        callback: extern "C" fn(
            time: u32,
            event: *mut fluid_event_t,
            seq: *mut fluid_sequencer_t, 
            data: *mut c_void),
        data: *mut c_void) -> FluidSeqId;
    pub fn fluid_sequencer_register_fluidsynth(
       seq: *mut fluid_sequencer_t,
       synth: *mut fluid_synth_t) -> FluidSeqId;
    pub fn fluid_sequencer_send_at(
        seq: *mut fluid_sequencer_t,
        evt: *mut fluid_event_t,
        time: u32,
        absolute: i32) -> i32;
    pub fn fluid_settings_setint(
        settings: *mut fluid_settings_t,
	name: *const i8,
	val: i32) -> i32;
    pub fn fluid_synth_program_select(
     	synth: *mut fluid_synth_t,
        chan: i32,
        sfont_id: i32,
        bank_num: i32,
        preset_num: i32) -> i32;
    pub fn fluid_synth_sfload(
        synth: *mut fluid_synth_t,
        path: *const i8,
        reset_presets: i32) -> i32;
    pub fn new_fluid_event() -> *mut fluid_event_t;
    pub fn new_fluid_settings() -> *mut fluid_settings_t;
    pub fn new_fluid_audio_driver(
        settings: *mut fluid_settings_t,
        synth: *mut fluid_synth_t) -> *mut fluid_audio_driver_t;
    pub fn new_fluid_sequencer2(use_system_timer: i32)
        -> *mut fluid_sequencer_t;
    pub fn new_fluid_synth(settings: *mut fluid_settings_t)
        -> *mut fluid_synth_t;
}
