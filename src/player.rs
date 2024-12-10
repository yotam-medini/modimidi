use std::{thread, time};
use std::fmt;
use std::os::raw::c_void;
use std::ffi::CString;
use crate::cfluid;
use crate::midi;
use crate::sequencer;

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
        println!("fluid_sequencer_send_at: date={}", date);
        let fluid_res = cfluid::fluid_sequencer_send_at(
            seq_ctl.sequencer_ptr, evt, date, 0); // 1 absolute, 0 relative
        println!("play_note: fluid_res={}", fluid_res);
        cfluid::delete_fluid_event(evt);
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

fn get_index_events(parsed_midi: &midi::Midi) -> Vec<IndexEvent> {
    let mut index_events = Vec::<IndexEvent>::new();
    for (ti, track) in parsed_midi.tracks.iter().enumerate() {
        let mut curr_time = 0;
        for (tei, track_event) in track.track_events.iter().enumerate() {
            let next_time = curr_time + track_event.delta_time;
            index_events.push(IndexEvent{ time: next_time, track: ti, tei: tei });
            curr_time = next_time;
        }
    }
    // println!("Before sort");
    // print_index_events(&index_events, parsed_midi);
    index_events.sort_by(|e0, e1| symmetric_cmp(e0, e1));
    println!("After sort");
    print_index_events(&index_events, parsed_midi);
    index_events
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
    round_div(numer, self.k_ticks_per_quarter)
  }
}

struct CallbackData<'a> {
    seq_ctl: &'a sequencer::SequencerControl,
    parsed_midi: &'a midi::Midi,
    index_events: &'a Vec<IndexEvent>,
    timing: &'a Timing,
    next_index_event: usize,
}

extern "C" fn seq_callback(
    time: u32,
    event: *mut cfluid::fluid_event_t,
    seq: *mut cfluid::fluid_sequencer_t, 
    data: *mut c_void) {
    println!("{}:{} seq_callback thread id={:?}", file!(), line!(), std::thread::current().id());
    unsafe {
        let cb_data = &mut *(data as *mut CallbackData);
        println!("seq_callback: {}:{} time={}, #(index_events)={}, next_index_event={}",
            file!(), line!(),
            time, cb_data.index_events.len(), cb_data.next_index_event);
    }
}

fn schedule_next_callback(seq_ctl : &mut sequencer::SequencerControl) {
    unsafe { 
      let sequencer_ptr = seq_ctl.sequencer_ptr;
      let now = cfluid::fluid_sequencer_get_tick(sequencer_ptr); 
      let evt = cfluid::new_fluid_event();
      cfluid::fluid_event_set_source(evt, -1);
      cfluid::fluid_event_set_dest(evt, seq_ctl.my_seq_id);
      println!("{}:{} now={}", file!(), line!(), now);
      let fluid_res = cfluid::fluid_sequencer_send_at(sequencer_ptr, evt, now + 100, 1);
      println!("{}:{} fluid_res={}", file!(), line!(), fluid_res);
      cfluid::delete_fluid_event(evt);
    }
}

pub fn play(seq_ctl: &mut sequencer::SequencerControl, parsed_midi: &midi::Midi) {
    println!("play... thread id={:?}", std::thread::current().id());
    let index_events = get_index_events(parsed_midi);
    unsafe {
        seq_ctl.now = cfluid::fluid_sequencer_get_tick(seq_ctl.sequencer_ptr);
        println!("play: tick={}", seq_ctl.now);
    }
    thread::sleep(time::Duration::from_millis(1000));

    // 1-tick = (microseconds_per_quarter / parsed_midi.ticks_per_quarter)/1000 milliseconds
    let mut timing = Timing {
      microseconds_per_quarter: 500000u64,
      k_ticks_per_quarter: 1000 * u64::from(parsed_midi.ticks_per_quarter_note), // SMPTE not yet
    };

    let callback_data = CallbackData {
        seq_ctl: seq_ctl,
        parsed_midi: parsed_midi,
        index_events: &index_events,
        timing: &timing,
        next_index_event: 0,
    };
    let callback_data_ptr = &callback_data as *const CallbackData as *mut c_void;
    let key = CString::new("me").expect("CString::new failed");
    let my_seq_id: i16;
    unsafe {
        my_seq_id = cfluid::fluid_sequencer_register_client(
            seq_ctl.sequencer_ptr, 
            key.as_ptr(),
            seq_callback, 
            callback_data_ptr);
    }
    seq_ctl.my_seq_id = my_seq_id;
    let t0;
    unsafe { t0 = cfluid::fluid_sequencer_get_tick(seq_ctl.sequencer_ptr); }
    println!("t0={}", t0);
    schedule_next_callback(seq_ctl);
    
    for (i, index_event) in index_events.iter().enumerate() {
       let track_event = &parsed_midi.tracks[index_event.track].track_events[index_event.tei];
       match track_event.event {
          midi::Event::MetaEvent(ref me) => {
              println!("i={}, MetaEvent={}", i, me);
              match me {
                  midi::MetaEvent::Text(e) => { println!("{}", e); },
                  midi::MetaEvent::SequenceTrackName(e) => { println!("{}", e); },
                  midi::MetaEvent::InstrumentName(e) => { println!("{}", e); },
                  midi::MetaEvent::EndOfTrack(_e) => {println!("EndOfTrack {}", index_event.track);},
                  midi::MetaEvent::SetTempo(st) => {
                      timing.microseconds_per_quarter = u64::from(st.tttttt);
                  },
                  midi::MetaEvent::TimeSignature(e) => { println!("{}", e); }
                  _ => { println!("play: unsupported");},
              }
          },
          midi::Event::MidiEvent(ref me) => {
              println!("i={}, MidiEvent={} ", i, me);
              match me {
                  midi::MidiEvent::NoteOn(ref e) => {
                      println!("{}", e); 
                      if e.velocity != 0 {
                          let duration_ticks = get_note_duration(parsed_midi, &index_events, i, e);
                          let duration_ms = timing.ticks_to_ms(duration_ticks);
                          let date_ms = timing.ticks_to_ms(index_event.time);
                          println!("duration_ticks={}, duration_ms={}", duration_ticks, duration_ms);
                          play_note(
                              seq_ctl, 
                              i32::from(e.channel),
                              i16::from(e.key),
                              i16::from(e.velocity),
                              duration_ms,
                              date_ms);
                      }
                  },
                  midi::MidiEvent::ProgramChange(e) => {
                      println!("{}", e);
                      let ret;
                      unsafe {
                          ret = cfluid::fluid_synth_program_select(
                              seq_ctl.synth_ptr,
                              i32::from(e.channel),
                              seq_ctl.sfont_id,
                              0,
                              i32::from(e.program));
                      }
                      if ret != cfluid::FLUID_OK {
                          eprintln!("fluid_synth_program_select failed ret={}", ret);
                      }
                  },
                  _ => { println!("play: unsupported");},
              }
          },
          _ => { },
       }
    }
    unsafe {
        let tick = cfluid::fluid_sequencer_get_tick(seq_ctl.sequencer_ptr);
        println!("before sleep tick={}", tick);
        // send_note_on(&mut sequencer, 0, 65, tick);
        thread::sleep(time::Duration::from_millis(5000));
        let tend = cfluid::fluid_sequencer_get_tick(seq_ctl.sequencer_ptr);
        println!("after sleep tick={}", tend);
    }
}
