use std::fmt;
use std::io::{self, Write};
use std::os::raw::c_void;
use std::ffi::CString;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, Condvar};
use crate::cfluid;
use crate::midi;
use crate::sequencer;
use crate::util;

fn play_note(
    seq_ctl: &mut sequencer::SequencerControl,
    chan: i32,
    key: i16,
    vel: i16,
    dur: u32,
    date: u32) {
    unsafe {
        let evt = cfluid::new_fluid_event();
        cfluid::fluid_event_set_source(evt, -1);
        cfluid::fluid_event_set_dest(evt, seq_ctl.synth_seq_id);
        cfluid::fluid_event_note(evt, chan, key, vel, dur);
        let fluid_res = cfluid::fluid_sequencer_send_at(
            seq_ctl.sequencer_ptr, evt, date, 1); // 1 absolute, 0 relative
        if fluid_res != cfluid::FLUID_OK {
            println!("play_note: fluid_res={}", fluid_res);
        }
        cfluid::delete_fluid_event(evt);
    }
}

fn send_final_event(seq_ctl: &mut sequencer::SequencerControl, date: u32) {
    unsafe {
        let evt = cfluid::new_fluid_event();
        cfluid::fluid_event_set_source(evt, -1);
        cfluid::fluid_event_set_dest(evt, seq_ctl.final_seq_id);
        let fluid_res = cfluid::fluid_sequencer_send_at(
            seq_ctl.sequencer_ptr, evt, date, 1); // 1 absolute, 0 relative
        if fluid_res != cfluid::FLUID_OK {
            eprintln!("fluid_sequencer_send_at failed ret={}", fluid_res);
        }
        cfluid::delete_fluid_event(evt);
    }
}

struct UserModification {
    begin_ms: u32,
    end_ms: u32,
    tempo_factor: f64,
}
impl fmt::Display for UserModification {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "UserModification([{}, {}] T={})", self.begin_ms, self.end_ms, self.tempo_factor)
    }
}

////////////////////////////////////////////////////////////////////////////////
struct IndexEvent {
    time: u32, // sum of delta_time
    track: usize,
    tei: usize,
}
impl fmt::Display for IndexEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "[tm={:6}, tr={}, e={:3}]", self.time, self.track, self.tei)
    }
}

fn symmetric_cmp(e0: &IndexEvent, e1: &IndexEvent) -> std::cmp::Ordering {
    let mut ord = std::cmp::Ordering::Equal;
    if e0.time == e1.time {
        if e0.track == e1.track {
            if e0.tei < e1.tei {
                ord = std::cmp::Ordering::Less;
            } else if e0.tei > e1.tei {
                ord = std::cmp::Ordering::Greater;
            }
        } else if e0.track < e1.track {
            ord = std::cmp::Ordering::Less;
        } else { // if e0.track > e1.track 
            ord = std::cmp::Ordering::Greater;
        }
    } else if e0.time < e1.time {
        ord = std::cmp::Ordering::Less;
    } else { // if (e0.time > e1.time
        ord = std::cmp::Ordering::Greater;
    }
    ord
}

fn print_index_events(index_events: &Vec<IndexEvent>, parsed_midi: &midi::Midi) {
    for (i, index_event) in index_events.iter().enumerate() {
       let event = &parsed_midi.tracks[index_event.track].track_events[index_event.tei].event;
       println!("[{:3}] {} {}", i, index_event, event); 
    }
}

fn get_index_events(parsed_midi: &midi::Midi, debug_flags: u32) -> Vec<IndexEvent> {
    let mut index_events = Vec::<IndexEvent>::new();
    for (ti, track) in parsed_midi.tracks.iter().enumerate() {
        let mut curr_time = 0;
        for (tei, track_event) in track.track_events.iter().enumerate() {
            let next_time = curr_time + track_event.delta_time;
            index_events.push(IndexEvent{ time: next_time, track: ti, tei: tei });
            curr_time = next_time;
        }
    }
    index_events.sort_by(|e0, e1| symmetric_cmp(e0, e1));
    if debug_flags & 0x20 != 0 {
        println!("index_events After sort");
        print_index_events(&index_events, parsed_midi);
    }
    index_events
}

struct NoteEvent {
    channel: i32,
    key: i16,
    velocity: i16,
    duration_ms: u32,
    duration_ms_original: u32,
}
impl fmt::Display for NoteEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "NoteEvent(c={:2}, key={:2}, vel={:3}, dur_ms={:4}, original={:4})",
            self.channel, self.key, self.velocity, self.duration_ms, self.duration_ms_original)
    }
}

struct ProgramChange {
    channel: i32,
    program: i32,
}
impl fmt::Display for ProgramChange {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "ProgramChange(channel={}, program={})", self.channel, self.program)
    }
}
struct PitchWheel {
    channel: i32,
    bend: i32,
}
impl fmt::Display for PitchWheel {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "PitchWheel(channel={}, bend={})", self.channel, self.bend)
    }
}
struct FinalEvent {
}
impl fmt::Display for FinalEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result { write!(f, "FinalEvent") }
}

enum UnionAbsEvent {
    NoteEvent(NoteEvent),
    ProgramChange(ProgramChange),
    PitchWheel(PitchWheel),
    FinalEvent(FinalEvent),
}
struct AbsEvent {
    time_ms: u32,
    time_ms_original: u32,
    uae: UnionAbsEvent,
}
impl fmt::Display for AbsEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "time_ms={} original={}, ", self.time_ms, self.time_ms_original);
        match &self.uae {
            UnionAbsEvent::NoteEvent(e) => write!(f, "{}", e),
            UnionAbsEvent::ProgramChange(e) => write!(f, "{}", e),
            UnionAbsEvent::PitchWheel(e) => write!(f, "{}", e),
            UnionAbsEvent::FinalEvent(e) => write!(f, "{}", e),
        }
    }
}

struct DynamicTiming {
    microseconds_per_quarter: u64,
    k_ticks_per_quarter: u64, // 1000 * ticks_per_quarter
    ticks_ref: u32,
    ms_ref: u32,
}
impl DynamicTiming {
    fn set_microseconds_per_quarter(&mut self, curr_ticks: u32, microseconds_per_quarter: u64) {
        self.ms_ref = self.abs_ticks_to_ms(curr_ticks);
        self.ticks_ref = curr_ticks;
        self.microseconds_per_quarter = microseconds_per_quarter;
    }
    fn ticks_to_ms(&self, ticks: u32) -> u32 {
        let numer = u64::from(ticks) * self.microseconds_per_quarter;
        let ret = round_div(numer, self.k_ticks_per_quarter);
        ret
    }
    fn abs_ticks_to_ms(&self, abs_ticks: u32) -> u32 {
        let numer = u64::from(abs_ticks - self.ticks_ref) * self.microseconds_per_quarter;
        let add = round_div(numer, self.k_ticks_per_quarter);
        let ret = self.ms_ref + add;
        ret
    }
}

fn max_by(var: &mut u32, val: u32) {
    if *var < val {
        *var = val;
    }
}

fn factor_u32(f: f64, u: u32) -> u32 {
    let ret: u32;
    let mult: f64 = f * (u as f64);
    let round = mult.round();
    if round > u32::MAX as f64 {
       eprint!("Overflow factor_u32(f={}, u={})", f, u);
       ret = u;
    } else {
       ret = round as u32;
    }
    ret
}

fn note_on_to_note_event(note_on: &midi::NoteOn, duration_ms: u32, factor: f64) -> NoteEvent {
    NoteEvent {
        channel: i32::from(note_on.channel),
        key: i16::from(note_on.key),
        velocity: i16::from(note_on.velocity),
        duration_ms: factor_u32(factor, duration_ms),
        duration_ms_original: duration_ms,
    }
}

struct AbsEventsGenerationControl<'a> {
    abs_events: Vec<AbsEvent>,
    final_ms: u32,
    parsed_midi: &'a midi::Midi,
    index_events: &'a Vec<IndexEvent>,
    user_mod: &'a UserModification,
    dynamic_timing: DynamicTiming,
}

fn abs_events_push(
    aed: &mut AbsEventsGenerationControl, 
    index_event_index: usize,
    me: &midi::MidiEvent,
    date_ms: u32,
    debug_flags: u32) {
    let factor = aed.user_mod.tempo_factor;
    let after_begin = aed.user_mod.begin_ms <= date_ms;
    let date_ms_modified = if after_begin {
        factor_u32(factor, date_ms - aed.user_mod.begin_ms)
    } else {
        aed.user_mod.begin_ms
    };
    match me {
        midi::MidiEvent::NoteOn(ref e) => {
            if after_begin && (e.velocity != 0) {
                let duration_ticks = get_note_duration(
                    aed.parsed_midi, aed.index_events, index_event_index, e);
                let duration_ms = aed.dynamic_timing.ticks_to_ms(duration_ticks);
                let note_event = note_on_to_note_event(e, duration_ms, factor);
                max_by(&mut aed.final_ms, date_ms_modified + note_event.duration_ms);
                aed.abs_events.push(AbsEvent{
                    time_ms: date_ms_modified,
                    time_ms_original: date_ms,
                    uae: UnionAbsEvent::NoteEvent(note_event)
                });
            }
        },
        midi::MidiEvent::NoteOff(ref _e) => {
            // pre-handled by calculating duration after NoteOn
        },
        midi::MidiEvent::ProgramChange(e) => {
            max_by(&mut aed.final_ms, date_ms_modified);
            let pc = ProgramChange { channel: i32::from(e.channel), program: i32::from(e.program), };
            aed.abs_events.push(AbsEvent{
                time_ms: date_ms_modified,
                time_ms_original: date_ms,
                uae: UnionAbsEvent::ProgramChange(pc)
            });
        },
        midi::MidiEvent::PitchWheel(e) => {
            max_by(&mut aed.final_ms, date_ms_modified);
            let pw = PitchWheel { channel: i32::from(e.channel), bend: i32::from(e.bend), };
            aed.abs_events.push(AbsEvent{
                time_ms: date_ms_modified,
                time_ms_original: date_ms,
                uae: UnionAbsEvent::PitchWheel(pw)
            });
        },
        _ => {
            if debug_flags & 0x100 != 0 {
                println!("{}:{} ignored: {}", file!(), line!(), me);
            }
        },
    }
}

fn get_first_note_time(control: &AbsEventsGenerationControl) -> u32 {
    let mut t: u32 = 0;
    let mut note_seen = false;
    let mut i: usize = 0;
    let tracks = &control.parsed_midi.tracks;
    while (i < control.index_events.len()) && !note_seen {
        let index_event = &control.index_events[i];
        let track_event = &tracks[index_event.track].track_events[index_event.tei];
        match track_event.event {
            midi::Event::MidiEvent(ref me) => {
                match me {
                    midi::MidiEvent::NoteOn(ref _e) => {
                        t = index_event.time;
                        note_seen = true;
                    }
                    _ => {},
                }
            },
            _ => {},
        }
        i += 1;
    }
    t
}

fn get_abs_events(
    parsed_midi: &midi::Midi,
    index_events: &Vec<IndexEvent>,
    user_mod: &UserModification,
    debug_flags: u32) -> Vec<AbsEvent> {
    let mut control = AbsEventsGenerationControl {
        abs_events: Vec::<AbsEvent>::new(),
        final_ms: 0,
        parsed_midi: parsed_midi,
        index_events: index_events,
        user_mod: user_mod,
        dynamic_timing: DynamicTiming {
            microseconds_per_quarter: 500000u64,
            k_ticks_per_quarter:
                1000 * u64::from(parsed_midi.ticks_per_quarter_note), // SMPTE not yet
            ticks_ref: 0,
            ms_ref: 0,
        },
    };
    let mut i: usize = 0;
    let first_note_time = get_first_note_time(&control);
    let mut done = false;
   if debug_flags & 0x80 != 0 { println!("#(index_events)={}", index_events.len()); }
    while (i < index_events.len()) && !done {
        let index_event = &index_events[i];
        let time_shifted = util::safe_subtract(index_event.time, first_note_time);
        let date_ms = control.dynamic_timing.abs_ticks_to_ms(time_shifted);
        done = date_ms > user_mod.end_ms;
        if !done {
            let track_event = &parsed_midi.tracks[index_event.track].track_events[index_event.tei];
            if debug_flags & 0x80 != 0 {
                println!("[{:3}] time={} shifted={}, track_event={}",
                    i, index_event.time, time_shifted, track_event); 
            }
            match track_event.event {
                midi::Event::MetaEvent(ref me) => {
                    match me {
                        midi::MetaEvent::SetTempo(st) => {
                            control.dynamic_timing.set_microseconds_per_quarter(
                                time_shifted, u64::from(st.tttttt));
                        },
                        _ => {
                           if debug_flags & 0x40 != 0 {
                               println!("{}:{} play: ignored: {}", file!(), line!(), me);
                           }
                        },
                    }
                },
                midi::Event::MidiEvent(ref me) => {
                    abs_events_push(&mut control, i, me, date_ms, debug_flags);
                },
                _ => { },
            }
            i += 1;
        }
    }
    if debug_flags & 0x200 != 0 {
        println!("final_ms={} == {}",
            control.final_ms, util::milliseconds_to_string(control.final_ms));
    }
    control.abs_events.push(AbsEvent {
        time_ms: std::cmp::max(control.final_ms, user_mod.begin_ms),
        time_ms_original: if control.abs_events.is_empty() {
            0
        } else {
            control.abs_events[control.abs_events.len() - 1].time_ms_original
        },
        uae: UnionAbsEvent::FinalEvent(FinalEvent{}),
    });
    if debug_flags & 0x200 != 0 {
        println!("abs_events:");
        for (i, ae) in control.abs_events.iter().enumerate() {
            println!("[{:4}] {}", i, ae);
        }
    }
    control.abs_events
}

fn get_note_duration(
    parsed_midi: &midi::Midi,
    index_events: &Vec<IndexEvent>,
    i: usize,
    note_on: &midi::NoteOn) -> u32 {
    let mut curr_event_time: u32 = 0;
    let mut end_note_found = false;
    let mut j = i + 1;
    while (!end_note_found) && (j < index_events.len()) {
        let index_event = &index_events[j];
        let track_event = &parsed_midi.tracks[index_event.track].track_events[index_event.tei];
        curr_event_time = index_event.time;
        match track_event.event {
            midi::Event::MidiEvent(ref me) => {
                match me {
                    midi::MidiEvent::NoteOn(e) => {
                        end_note_found = (e.velocity == 0) &&
                            (e.channel == note_on.channel) && (e.key == note_on.key);
                    },
                    midi::MidiEvent::NoteOff(e) => {
                        end_note_found = (e.channel == note_on.channel) && (e.key == note_on.key);
                    },
                    _ => {},
                }
            },
            _ => {},
        }
        j += 1;
    }
    let duration = curr_event_time - index_events[i].time;
    duration
}

fn round_div(n: u64, d: u64) -> u32 {
    let q: u64 = (n + d/2) / d;
    if q > u64::from(u32::MAX) {
        eprintln!("overflow @ round_div({}, {})", n, d);
    }
    let ret : u32 = q as u32;
    ret
}

struct Timing {
  microseconds_per_quarter: u64,
  k_ticks_per_quarter: u64, // 1000 * ticks_per_quarter
}
impl Timing {
  fn ticks_to_ms(&self, ticks: u32) -> u32 {
    let numer = u64::from(ticks) * self.microseconds_per_quarter;
    let ret = round_div(numer, self.k_ticks_per_quarter);
    // println!("Timing: μsecper♩={}, ticks={}, ms={}", self.k_ticks_per_quarter, ticks, ret);
    ret
  }
}

struct CallbackData<'a> {
    seq_ctl: &'a mut sequencer::SequencerControl,
    abs_events: &'a Vec<AbsEvent>,
    next_abs_event: usize,
    user_mod: &'a UserModification,
    factor_begin: u32,
    div_factor: f64,
    sending_events: AtomicBool,
    final_callback_handled: AtomicBool,
    mtx_cvar: Arc<(Mutex<bool>, Condvar)>
}

impl<'a> CallbackData<'a> {
  fn all_events_sent(&self) -> bool {
    self.next_abs_event == self.abs_events.len()
  }
}

fn send_next_batch_events(cb_data: &mut CallbackData) -> bool {
    let now;
    unsafe { now = cfluid::fluid_sequencer_get_tick(cb_data.seq_ctl.sequencer_ptr); }
    if cb_data.next_abs_event == 0 {
        cb_data.seq_ctl.add_ms = now + cb_data.seq_ctl.initial_delay_ms;
    }
    let mut final_event = false;
    while cb_data.next_abs_event < cb_data.abs_events.len() {
        let abs_event = &cb_data.abs_events[cb_data.next_abs_event];
        let at_ms = abs_event.time_ms + cb_data.seq_ctl.add_ms;
        match &abs_event.uae {
            UnionAbsEvent::NoteEvent(note_event) => {
                play_note(
                    cb_data.seq_ctl, 
                    note_event.channel,
                    note_event.key,
                    note_event.velocity,
                    note_event.duration_ms,
                    at_ms);
            },
            UnionAbsEvent::ProgramChange(program_change) => {
                let ret;
                unsafe {
                    ret = cfluid::fluid_synth_program_select(
                        cb_data.seq_ctl.synth_ptr,
                        i32::from(program_change.channel),
                        cb_data.seq_ctl.sfont_id,
                        0,
                        i32::from(program_change.program));
                }
                if ret != cfluid::FLUID_OK {
                    eprintln!("fluid_synth_program_select failed ret={}", ret);
                }
            },
            UnionAbsEvent::PitchWheel(pitch_wheel) => {
		unsafe {
                    let ret = cfluid::fluid_synth_pitch_bend(
		        cb_data.seq_ctl.synth_ptr,
                        pitch_wheel.channel,
                        pitch_wheel.bend);
                    if ret != cfluid::FLUID_OK {
                        eprintln!("fluid_synth_pitch_bend failed ret={}", ret);
                    }
                }
            }
            UnionAbsEvent::FinalEvent(_e) => { // must be the last event
                unsafe {
                    cfluid::fluid_sequencer_unregister_client(
                        cb_data.seq_ctl.sequencer_ptr, cb_data.seq_ctl.periodic_seq_id);
                }
                send_final_event(cb_data.seq_ctl, at_ms);
                final_event = true;
            }
        }
        cb_data.next_abs_event += 1;
    }
    final_event
}

fn handle_next_batch_events(cb_data: &mut CallbackData) -> bool {
   let mut ret = false;
   let already_sending = cb_data.sending_events.swap(true, Ordering::SeqCst);
   if !already_sending {
       ret = send_next_batch_events(cb_data);
       cb_data.sending_events.store(false, Ordering::SeqCst);
   }
   ret
}

extern "C" fn periodic_callback(
    _time: u32,
    _event: *mut cfluid::fluid_event_t,
    _seq: *mut cfluid::fluid_sequencer_t, 
    data: *mut c_void) {
    unsafe {
        let cb_data = &mut *(data as *mut CallbackData);
        // libfluidsynth in its fluid_sequencer_unregister_client(...) !!
        // call the callback (if any), to free underlying memory (e.g. seqbind structure)
        // so
        if !cb_data.all_events_sent() {
            let final_event_sent = handle_next_batch_events(cb_data);
            if !final_event_sent {
                let now = cfluid::fluid_sequencer_get_tick(cb_data.seq_ctl.sequencer_ptr);
                schedule_next_callback(cb_data.seq_ctl, now + cb_data.seq_ctl.batch_duration_ms/2);
            }
        }
    }
}

extern "C" fn final_callback(
    time: u32,
    _event: *mut cfluid::fluid_event_t,
    _seq: *mut cfluid::fluid_sequencer_t, 
    data: *mut c_void) {
    println!("{}:{} time={}", file!(), line!(), time);
    println!("{}:{} final_callback thread id={:?}", file!(), line!(), std::thread::current().id());
    unsafe {
        let cb_data = &mut *(data as *mut CallbackData);
        println!("final_callback: {}:{} time={}, #(abs_events)={}, next_abs_event={}",
            file!(), line!(),
            time, cb_data.abs_events.len(), cb_data.next_abs_event);
        let handled = cb_data.final_callback_handled.swap(true, Ordering::SeqCst);
        println!("final_callback: handled={}", handled);
        if !handled {
            println!("{}:{} thread={:?} all events played",
                file!(), line!(), std::thread::current().id());
            let mtx_cvar = Arc::clone(&cb_data.mtx_cvar); // Clone the Arc
            let (lock, cvar) = &*mtx_cvar;
            let mut lock_guard = lock.lock().unwrap();
            *lock_guard = true; // Example usage
            cvar.notify_all(); // Notify threads
            println!("{}:{} mtx_cvar={:?}", file!(), line!(), cb_data.mtx_cvar);
        }
    }
}

extern "C" fn progress_callback(
    time: u32,
    _event: *mut cfluid::fluid_event_t,
    _seq: *mut cfluid::fluid_sequencer_t, 
    data: *mut c_void) {
    unsafe {
        let cb_data = &mut *(data as *mut CallbackData);
        if !cb_data.final_callback_handled.load(Ordering::SeqCst) {
            let mut stdout = io::stdout();
            if let Some(final_event) = cb_data.abs_events.last() {
                let last_ms = final_event.time_ms_original;
                if time >= cb_data.seq_ctl.add_ms {
                    let dt = time - cb_data.seq_ctl.add_ms;
                    let dt_div_f = cb_data.div_factor * (dt as f64);
                    let btime = (dt_div_f as u32) + cb_data.user_mod.begin_ms;
                    if cb_data.seq_ctl.add_ms <= btime && btime <= last_ms {
                        let mmss_done = util::milliseconds_to_string(btime - cb_data.seq_ctl.add_ms);
                        let mmss_final = util::milliseconds_to_string(last_ms);
                        write!(stdout, "\rProgress: {} / {}", mmss_done, mmss_final);
                        let _ = stdout.flush();
                    }
                }
                schedule_next_progress_callback(cb_data.seq_ctl, time + 100); // every second/10
            }
        }
    }
}

fn schedule_seqid_callback(seq_ctl : &mut sequencer::SequencerControl, date_ms: u32, seq_id: i16) {
    // println!("{}:{} date_ms={}", file!(), line!(), date_ms);
    unsafe { 
      let sequencer_ptr = seq_ctl.sequencer_ptr;
      let evt = cfluid::new_fluid_event();
      cfluid::fluid_event_set_source(evt, -1);
      cfluid::fluid_event_set_dest(evt, seq_id);
      let fluid_res = cfluid::fluid_sequencer_send_at(sequencer_ptr, evt, date_ms, 1);
      if fluid_res != cfluid::FLUID_OK {
          eprintln!("{}:{} date_ms={}, fluid_res={}", file!(), line!(), date_ms, fluid_res);
      }
      cfluid::delete_fluid_event(evt);
    }
}

fn schedule_next_callback(seq_ctl : &mut sequencer::SequencerControl, date_ms: u32) {
    schedule_seqid_callback(seq_ctl, date_ms, seq_ctl.periodic_seq_id);
}

fn schedule_next_progress_callback(seq_ctl : &mut sequencer::SequencerControl, date_ms: u32) {
    // println!("{}:{} date_ms={}", file!(), line!(), date_ms);
    schedule_seqid_callback(seq_ctl, date_ms, seq_ctl.progress_seq_id);
}

pub fn play(
    seq_ctl: &mut sequencer::SequencerControl,
    parsed_midi: &midi::Midi,
    begin: u32,
    end: u32,
    tempo_factor: f64,
    progress: bool,
    debug_flags: u32,
    ) {
    if debug_flags & 0x10 != 0 { println!("play... thread id={:?}", std::thread::current().id()); }
    let index_events = get_index_events(parsed_midi, debug_flags);
    let user_mod = UserModification {
        begin_ms: begin,
        end_ms: end,
        tempo_factor: tempo_factor,
    };
    let abs_events = get_abs_events(parsed_midi, &index_events, &user_mod, debug_flags);

    // 1-tick = (microseconds_per_quarter / parsed_midi.ticks_per_quarter)/1000 milliseconds
    let timing = Timing {
      microseconds_per_quarter: 500000u64,
      k_ticks_per_quarter: 1000 * u64::from(parsed_midi.ticks_per_quarter_note), // SMPTE not yet
    };

    let mtx_cvar = Arc::new((Mutex::new(false), Condvar::new()));
    let t0;
    unsafe { t0 = cfluid::fluid_sequencer_get_tick(seq_ctl.sequencer_ptr); }
    let t0_ms = timing.ticks_to_ms(t0);
    let fb = factor_u32(user_mod.tempo_factor, user_mod.begin_ms);
    let callback_data = CallbackData {
        seq_ctl: seq_ctl,
        abs_events: &abs_events,
        next_abs_event: 0,
        user_mod: &user_mod,
        factor_begin: fb,
        div_factor: 1.0 / user_mod.tempo_factor,
        sending_events: AtomicBool::new(false),
        final_callback_handled: AtomicBool::new(false),
        mtx_cvar: Arc::clone(&mtx_cvar),
    };
    let callback_data_ptr = &callback_data as *const CallbackData as *mut c_void;
    let key_periodic = CString::new("periodic").expect("CString::new failed");
    let key_final = CString::new("final").expect("CString::new failed");
    let key_progress = CString::new("progress").expect("CString::new failed");
    unsafe {
        seq_ctl.periodic_seq_id = cfluid::fluid_sequencer_register_client(
            seq_ctl.sequencer_ptr, 
            key_periodic.as_ptr(),
            periodic_callback, 
            callback_data_ptr);
        seq_ctl.final_seq_id = cfluid::fluid_sequencer_register_client(
            seq_ctl.sequencer_ptr, 
            key_final.as_ptr(),
            final_callback, 
            callback_data_ptr);
        if progress {
            seq_ctl.progress_seq_id = cfluid::fluid_sequencer_register_client(
                seq_ctl.sequencer_ptr, 
                key_progress.as_ptr(),
                progress_callback,
                callback_data_ptr);
        }
    }
    schedule_next_callback(seq_ctl, t0_ms);
    if progress {
        schedule_next_progress_callback(seq_ctl, t0_ms);
    }

    let (lock, cvar) = &*mtx_cvar;
    let mut locked = lock.lock().unwrap();
    if debug_flags & 0x10 != 0 {
        println!("{}:{} Waiting on locked thread={:?}",
            file!(), line!(), std::thread::current().id());
    }
    let mut locked_loop: u64 = 0;
    while !*locked {
        locked_loop += 1;
        locked = cvar.wait(locked).unwrap();
    }
    println!("{}:{} Got notification! thread={:?}", file!(), line!(), std::thread::current().id());
    println!("locked_loop={}", locked_loop);
    if false {
        unsafe {
            cfluid::fluid_sequencer_unregister_client(seq_ctl.sequencer_ptr, seq_ctl.periodic_seq_id);
            cfluid::fluid_sequencer_unregister_client(seq_ctl.sequencer_ptr, seq_ctl.final_seq_id);
        }
    }
}
