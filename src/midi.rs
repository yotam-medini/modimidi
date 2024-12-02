use std::cmp;
use std::fmt;
use std::path::PathBuf;
use std::fs;

pub struct NoteOn {
    channel: u8,
    key: u8,
    velocity: u8,
}
impl fmt::Display for NoteOn {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "NotOn(channel={}, key={}, velocity={})",
            self.channel, self.key, self.velocity)
    }
}

pub enum MidiEvent {
    NoteOn(NoteOn),
    Undef,
}

pub struct SysexEvent {
}

pub struct Text {
    name: String,
}
impl fmt::Display for Text {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Text(name={})", self.name)
    }
}

pub struct SequenceTrackName {
    name: String,
}
impl fmt::Display for SequenceTrackName {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "SequenceTrackName(name={})", self.name)
    }
}

pub struct TimeSignature {
    nn: u8, // nunmerator
    dd: u8, // negative power of 2, denominator
    cc: u8, // MIDI clocks in a metronome click
    bb: u8, // number of notated 32nd-notes in a MIDI quarter-note
}
impl fmt::Display for TimeSignature {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "TimeSignature(nn={}, dd={}, cc={}, bb={})", self.nn, self.dd, self.cc, self.bb)
    }
}

pub struct SetTempo {
    tttttt: u32, // microseconds per MIDI quarter-note
}
impl fmt::Display for SetTempo {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "SetTempo(tttttt={})", self.tttttt)
    }
}

pub struct EndOfTrack {
}

pub enum MetaEvent {
    Text(Text),
    SequenceTrackName(SequenceTrackName),
    TimeSignature(TimeSignature),
    SetTempo(SetTempo),
    EndOfTrack(EndOfTrack),
    Undef,
}
impl fmt::Display for MetaEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            MetaEvent::Text(t) => write!(f, "{}", t),
            MetaEvent::SequenceTrackName(name) => write!(f, "{}", name),
            MetaEvent::TimeSignature(ts) => write!(f, "{}", ts),
            MetaEvent::SetTempo(st) => write!(f, "{}", st),
            MetaEvent::EndOfTrack(eot) => write!(f, "EndOfTrack"),
            MetaEvent::Undef => write!(f, "Undef"),
        }
    }
}
pub enum Event {
    MidiEvent(MidiEvent),
    SysexEvent(SysexEvent),
    MetaEvent(MetaEvent),
    Undef,
}
impl fmt::Display for Event {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Event::MidiEvent(me) => write!(f, "MidiEvent::"),
            Event::SysexEvent(se) => write!(f, "SysexEvent"),
            Event::MetaEvent(me) => write!(f, "MetaEvent::{}", me),
            Event::Undef => write!(f, "Undef"),
        }
    }
}

pub struct TrackEvent {
    delta_time: u32,
    event: Event,
}
impl fmt::Display for TrackEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "TrackEvent(delta_time={}, event={}", self.delta_time, self.event)
    }
}

pub struct Track {
    track_events: Vec<TrackEvent>,
}
impl fmt::Display for Track {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "events[{}]: {}", self.track_events.len(), "{\n")?;
        for (i, te) in self.track_events.iter().enumerate() {
            write!(f, "    track_event=[{}]: {},\n", i, te)?;
        }
        write!(f, "{}", "  }")?;
        Ok(())
    }
}

pub struct Midi {
    error: String,
    format: u16,
    ntrks: u16,
    ticks_per_quarter_note: u16,
    negative_smpte_format: u8,
    ticks_per_frame: u8,
    tracks: Vec<Track>,
}
impl fmt::Display for Midi {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Midi{}format={}, ntrks={}, ", "{", self.format, self.ntrks)?;
        if self.ticks_per_quarter_note != 0 {
            write!(f, "ticks_per_quarter_note={:?}", self.ticks_per_quarter_note)?;
        } else {
            write!(f, "negative_smpte_format={}, ticks_per_frame={}",
                self.negative_smpte_format, self.ticks_per_frame)?;
        }
        write!(f, ", tracks:\n")?;
        for (i, track) in self.tracks.iter().enumerate() {
            write!(f, "  track=[{}]: {},\n", i, track);
        }
        write!(f, "{}", "}")?;
        Ok(())
    }
}

fn get_usize(data: &Vec<u8>, offset: &mut usize) -> usize {
    let offs: usize = *offset;
    let ret: usize = 
        (usize::from(data[offs + 0]) << (3*8)) |
        (usize::from(data[offs + 1]) << (2*8)) |
        (usize::from(data[offs + 2]) << (1*8)) |
        (usize::from(data[offs + 3]));
    *offset = offs + 4;
    ret
}

fn get_chunk_type(data: &Vec<u8>, offset: &mut usize) -> String {
    let mut chunk_type = String::new();
    let next_offset: usize = *offset + 4;
    for i in *offset..next_offset {
        let cdata: char = char::from_u32(u32::from(data[i])).unwrap();
        chunk_type.push(cdata);
    }
    *offset = next_offset;
    chunk_type
}

fn get_variable_length_quantity(data: &Vec<u8>, offset: &mut usize) -> u32 {
    let mut quantity: u32 = 0;
    let mut offs: usize = *offset;
    let mut done = false;
    let offs_limit = offs + 4;
    while (offs < offs_limit) && !done {
        let b: u8 = data[offs];
        println!("offs={}, b={:02x}", offs, b);
        quantity = (quantity << 7) + (u32::from(b) & 0x7f);
        done = (b & 0x80) == 0;
        offs += 1;
    }
    *offset = offs;
    quantity
}

fn get_sized_quantity(data: &Vec<u8>, offset: &mut usize) -> u32 {
    let offs: usize = *offset + 2;
    let n_bytes = data[offs] as usize;
    let mut quantity: u32 = 0;
    for i in offs+1..offs+1+n_bytes {
        quantity = (quantity << 8) + u32::from(data[i]);
    }
    *offset = offs + 1 + n_bytes;
    quantity
}

fn get_string(data: &Vec<u8>, offset: &mut usize, length: u32) -> String {
    let mut text = String::new();
    let next_offset: usize = *offset + (length as usize);
    for i in *offset..next_offset {
        let cdata: char = char::from_u32(u32::from(data[i])).unwrap();
        text.push(cdata);
    }
    *offset = next_offset;
    println!("get_string: text={}", text);
    text
}

fn get_track_event(data: &Vec<u8>, offset: &mut usize) -> TrackEvent {
    let delta_time = get_variable_length_quantity(data, offset);
    println!("delta_time={}, offset={}", delta_time, offset);
    let event_first_byte = data[*offset];
    println!("event_first_byte={:#02x}", event_first_byte);
    let mut te = TrackEvent {
        delta_time: delta_time,
        event: Event::Undef,
    };
    match event_first_byte {
        0xff => { // Meta Event
            println!("meta... {:#02x} {:#02x}", data[*offset + 1], data[*offset + 2]);
            let meta_event = get_meta_event(data, offset);
            println!("offset={}", offset);
            te.event = Event::MetaEvent(meta_event);
        },
        0xf0 | 0xf7 => { // Sysex Event
            println!("Sysex Event")
        },
        _ => { // Midi Event
            println!("midi event... {:#02x} {:#02x}", data[*offset + 1], data[*offset + 2]);
            let midi_event = get_midi_event(data, offset);
            println!("offset={}", offset);
            te.event = Event::MidiEvent(midi_event);
        }
    }
    te
}

fn get_midi_event(data: &Vec<u8>, offset: &mut usize) -> MidiEvent {
    let offs = *offset;
    let mut midi_event = MidiEvent::Undef;
    let upper4 = (data[offs] >> 4) & 0xff;
    match upper4 {
        0x9 => {
            let note_on = NoteOn {
                channel: data[offs] & 0xf,
                key: data[offs + 1],
                velocity: data[offs + 2],
            };
            println!("note_on={}", note_on);
            midi_event = MidiEvent::NoteOn(note_on);
            *offset = offs + 3;
        }
        _ => {
            eprintln!("Unsupported upper4={:x}", upper4);
        },
    }
    midi_event
}

fn get_meta_event(data: &Vec<u8>, offset: &mut usize) -> MetaEvent {
    let offs = *offset;
    assert!(data[offs] == 0xff);
    let mut seq_track_name = SequenceTrackName {
        name: String::new(),
    };
    let mut meta_event = MetaEvent::Undef;
    match data[offs + 1] {
        0x01 => {
            *offset = offs + 2; 
            let length = get_variable_length_quantity(data, offset);
            println!("length={}", length);
            let text = get_string(data, offset, length);
            let e = Text {
               name: text,
            };
            meta_event = MetaEvent::Text(e);
        },
        0x03 => {
            *offset = offs + 2; 
            let length = get_variable_length_quantity(data, offset);
            let text = get_string(data, offset, length);
            seq_track_name = SequenceTrackName {
               name: text,
            };
            meta_event = MetaEvent::SequenceTrackName(seq_track_name);
        },
        0x2f => {
            let n_bytes: usize = usize::from(data[offs + 2]);
            if n_bytes != 0 {
                println!("Unexpected n_bytes={} in EndOfTrack", n_bytes);
            }
            meta_event = MetaEvent::EndOfTrack(EndOfTrack {});
            *offset = offs + 3 + n_bytes;
        },
        0x51 => {
            let set_tempo = SetTempo { tttttt: get_sized_quantity(data, offset), };
            meta_event = MetaEvent::SetTempo(set_tempo);
        },
        0x58 => {
            if data[offs + 2] != 0x04 {
                eprintln!("Unexpected byte {:02x} followeing 0x58 TimeSignature meta event",
                    data[offs + 2]);
            }
            let time_signature = TimeSignature {
                nn: data[offs + 3],
                dd: data[offs + 4],
                cc: data[offs + 5],
                bb: data[offs + 6]
            };
            meta_event = MetaEvent::TimeSignature(time_signature);
            *offset = offs + 7;
        },
        _ => { 
            eprintln!("Not yet supported MetaEvent {:#02x}", data[offs + 1]);
        },
    }
    meta_event
}

impl Midi {
    pub fn ok(&self) -> bool {
        self.error.is_empty()
    }
    pub fn set_error(&mut self, err: String) {
        if self.ok() {
            eprintln!("{}", err);
            self.error = err;
        }
    }
    fn read_one_track(&mut self, data: &Vec<u8>, offset: &mut usize) {
        self.read_track(&data, offset);
    }
    fn read_tracks(&mut self, data: &Vec<u8>, offset: &mut usize) {
        for itrack in 0..self.ntrks {
            println!("itrack={}, offset={}", itrack, offset);
            if self.ok() {
                self.read_track(&data, offset);
            }
        }
    }
    fn read_track(&mut self, data: &Vec<u8>, offset: &mut usize) {
        const MTRK: &str = "MTrk";
        println!("read_track: offset={}", offset);
        let chunk_type = get_chunk_type(data, offset);
        println!("read_track: chunk_type={}, offset={}", chunk_type, offset);
        if chunk_type != MTRK {
            self.set_error(format!("chunk_type={} != {} @ offset={}",
                chunk_type, MTRK, offset));
        } else {
            let offset_begin_of_track = *offset;
            let length = get_usize(&data, offset);
            let offset_eot = *offset + length;
            println!("length={}, offset={}, eot={}", length, offset, offset_eot);
            let mut track = Track {
                track_events: Vec::<TrackEvent>::new(),
            };
            let mut eot = false;
            while (!eot) & (*offset < offset_eot) {
                let track_event = get_track_event(data, offset);
                track.track_events.push(track_event);
            }
            self.tracks.push(track);
            *offset = offset_eot;
        }
    }
}

pub fn parse_midi_file(filename: &PathBuf) -> Midi {
    println!("parse_midi_file({:?})", filename);
    let mut midi = Midi {
        error: String::new(),
        format: 0xffff,
        ntrks: 0,
        ticks_per_quarter_note: 0,
        negative_smpte_format: 0,
        ticks_per_frame: 0,
        tracks: Vec::<Track>::new(),
    };
    let mut length: usize = 0;
    let meta = fs::metadata(filename);
    let mut file_size: u64 = 0;
    match meta {
        Ok(mt) => { println!("mt={:?}", mt); file_size = mt.len(); },
        Err(e) => {
            println!("Error {:?}", e); 
            midi.set_error(format!("Error {:?}", e));
        }
    }
    println!("{:?} size={}", filename, file_size);
    let data: Vec<u8> =
        if midi.ok() {fs::read(filename).unwrap() } else { Vec::<u8>::new() };
    if midi.ok() {
        println!("#(data)={}", data.len());
        for w in 0..cmp::min(0x20,(file_size/4) as usize) {
            let mut s4 = String::new();
            for i in 0..4 {
                let mut c: char = ' ';
                let u8 = data[4*w + i];
                if (0x20 <= u8) && (u8 <= 0x7f) {
                    c = char::from_u32(u32::from(u8)).unwrap();
                }
                s4.push(c);
            }
            println!(
                "data[{:03}]: {:#010b} {:#010b} {:#010b} {:#010b}  {:02x} {:02x} {:02x} {:02x} {}",
                4*w,
                data[4*w + 0], data[4*w + 1], data[4*w + 2], data[4*w + 3],
                data[4*w + 0], data[4*w + 1], data[4*w + 2], data[4*w + 3],
                s4);
        }
        // last bytes
        let begin_of_last_quad: usize = 4*(data.len()/4);
        for i in begin_of_last_quad..data.len() {
            println!("data[{}]={:02x}", i, data[usize::from(i)]);
        }
        let mut offset = 0;
        const MTHD: &str = "MThd";
        let mthd = get_chunk_type(&data, &mut offset);
        println!("mthd={}", mthd);
        if mthd != MTHD {
            midi.set_error(format!("Header chunk: {} != {}", mthd, MTHD));
        }
    }
    if midi.ok() {
        let mut offset: usize = 4;
        length = get_usize(&data, &mut offset);
        midi.format = (u16::from(data[8]) << 8) | u16::from(data[9]);
        midi.ntrks = (u16::from(data[10]) << 8) | u16::from(data[11]);
        println!("length={}, format={}, ntrks={}",
            length, midi.format, midi.ntrks);
        if length != 6 {
            println!("Unexpected length: {} != 6", length);
        }
        let division : u16 = (u16::from(data[12]) << 8) | u16::from(data[13]);
        println!("division={:#018b}", division); // division=0b0000000110000000
        let bit15: u16 = division >> 15;
        if bit15 == 0 {
            midi.ticks_per_quarter_note = division;
        } else {
            midi.negative_smpte_format = data[12] & 0x7f;
            midi.ticks_per_frame = data[13];
        }
        println!("ticks_per_quarter_note={}", midi.ticks_per_quarter_note);
        println!("ticks_per_frame={}", midi.ticks_per_frame);
        println!("negative_smpte_format={}", midi.negative_smpte_format);
    }
    if midi.ok() {
        let mut offset: usize = 4 + 4 + length;
        match midi.format {
            0 => midi.read_one_track(&data, &mut offset),
            1|2 => midi.read_tracks(&data, &mut offset),
            _ => midi.set_error(format!("Unsupported midi format: {}",
                midi.format))
        }
    }
    return midi;
}
