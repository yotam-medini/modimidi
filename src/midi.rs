use std::cmp;
use std::fmt;
use std::path::PathBuf;
use std::fs;

pub struct NoteOff { // 0x8?
    pub channel: u8,
    pub key: u8,
    pub velocity: u8,
}
impl fmt::Display for NoteOff {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "NoteOff(channel={}, key={}, velocity={})",
            self.channel, self.key, self.velocity)
    }
}

pub struct NoteOn { // 0x9?
    pub channel: u8,
    pub key: u8,
    pub velocity: u8,
}
impl fmt::Display for NoteOn {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "NoteOn(channel={}, key={}, velocity={})",
            self.channel, self.key, self.velocity)
    }
}

pub struct ControlChange { // 0xb
    channel: u8,
    number: u8,
    value: u8,
}
impl fmt::Display for ControlChange {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "ControlChange(channel={}, number={}, value={})",
            self.channel, self.number, self.value)
    }
}

pub struct ProgramChange { // 0xc
    pub channel: u8,
    pub program: u8,
}
impl fmt::Display for ProgramChange {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "ProgramChange(channel={}, program={})",
            self.channel, self.program)
    }
}

pub enum MidiEvent {
    NoteOff(NoteOff),
    NoteOn(NoteOn),
    ControlChange(ControlChange),
    ProgramChange(ProgramChange),
    Undef,
}
impl fmt::Display for MidiEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            MidiEvent::ProgramChange(pc) => write!(f, "{}", pc),
            MidiEvent::ControlChange(cc) => write!(f, "{}", cc),
            MidiEvent::NoteOff(note_off) => write!(f, "{}", note_off),
            MidiEvent::NoteOn(note_on) => write!(f, "{}", note_on),
            MidiEvent::Undef => write!(f, "Undef"),
        }
    }
}

pub struct SysexEvent {
}

pub struct Text { // 0xff 0x01
    name: String,
}
impl fmt::Display for Text {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Text(name={})", self.name)
    }
}

pub struct SequenceTrackName { // 0xff 0x03
    name: String,
}
impl fmt::Display for SequenceTrackName {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "SequenceTrackName(name={})", self.name)
    }
}

pub struct InstrumentName  { // 0xff 0x04
    name: String,
}
impl fmt::Display for InstrumentName {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "InstrumentName(name={})", self.name)
    }
}

pub struct EndOfTrack { // 0xff 0x2f
}

pub struct SetTempo { // 0xff 0x51
    pub tttttt: u32, // microseconds per MIDI quarter-note
}
impl fmt::Display for SetTempo {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "SetTempo(tttttt={})", self.tttttt)
    }
}

pub struct TimeSignature { // 0xff 0x58
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

pub struct KeySignature { // 0xff 0x59
    sf: i16, // -7..+7 number of flats or sharps
    mi: bool, // is Minor 
}
impl KeySignature {
    fn scale_name(&self) -> String {
        let quintes = "CGDAEBF";
        let qi = ((self.sf + 7 + (if self.mi {3} else {0})) % 7) as usize;
        let tonica = quintes.chars().nth(qi).unwrap();
        let scale_name = format!("{} m{}or", tonica, if self.mi {"in"} else {"aj"});
        scale_name
    }
}
impl fmt::Display for KeySignature {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "KeySignature(sf={}, mi={}, [{}]", self.sf, self.mi, self.scale_name())
    }
}

pub struct SequencerEvent { // 0xff 0x7f
    data: Vec<u8>,
}
impl fmt::Display for SequencerEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "SequencerEvent({:?})", self.data)
    }
}

pub enum MetaEvent {
    Text(Text),
    SequenceTrackName(SequenceTrackName),
    InstrumentName(InstrumentName),
    EndOfTrack(EndOfTrack),
    SetTempo(SetTempo),
    TimeSignature(TimeSignature),
    KeySignature(KeySignature),
    SequencerEvent(SequencerEvent),
    Undef,
}
impl fmt::Display for MetaEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            MetaEvent::Text(t) => write!(f, "{}", t),
            MetaEvent::SequenceTrackName(name) => write!(f, "{}", name),
            MetaEvent::InstrumentName(iname) => write!(f, "{}", iname),
            MetaEvent::EndOfTrack(_eot) => write!(f, "EndOfTrack"),
            MetaEvent::SetTempo(st) => write!(f, "{}", st),
            MetaEvent::TimeSignature(ts) => write!(f, "{}", ts),
            MetaEvent::KeySignature(ks) => write!(f, "{}", ks),
            MetaEvent::SequencerEvent(se) => write!(f, "{}", se),
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
            Event::MidiEvent(me) => write!(f, "MidiEvent::{}", me),
            Event::SysexEvent(_se) => write!(f, "SysexEvent"),
            Event::MetaEvent(me) => write!(f, "MetaEvent::{}", me),
            Event::Undef => write!(f, "Undef"),
        }
    }
}

pub struct TrackEvent {
    pub delta_time: u32,
    pub event: Event,
}
impl fmt::Display for TrackEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "TrackEvent(delta_time={}, event={}", self.delta_time, self.event)
    }
}

pub struct Track {
    pub track_events: Vec<TrackEvent>,
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
    pub ticks_per_quarter_note: u16,
    negative_smpte_format: u8,
    ticks_per_frame: u8,
    pub tracks: Vec<Track>,
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
            write!(f, "  track=[{}]: {},\n", i, track)?;
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
            println!("midi event... {:#02x} {:#02x} {:#02x}",
                data[*offset + 0], data[*offset + 1], data[*offset + 2]);
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
        0x8 => {
            let note_off = NoteOff {
                channel: data[offs] & 0xf,
                key: data[offs + 1],
                velocity: data[offs + 2],
            };
            println!("note_off={}", note_off);
            midi_event = MidiEvent::NoteOff(note_off);
            *offset = offs + 3;
        },
        0x9 => {
            let note_on = NoteOn {
                channel: data[offs] & 0xf,
                key: data[offs + 1],
                velocity: data[offs + 2],
            };
            println!("note_on={}", note_on);
            midi_event = MidiEvent::NoteOn(note_on);
            *offset = offs + 3;
        },
        0xb => {
            let cc = ControlChange{
                channel: data[offs] & 0xf,
                number: data[offs + 1],
                value: data[offs + 2],
            };
            midi_event = MidiEvent::ControlChange(cc);
            *offset = offs + 3;
        },
        0xc => {
            let pc = ProgramChange{channel: data[offs] & 0xf, program: data[offs + 1]};
            midi_event = MidiEvent::ProgramChange(pc);
            *offset = offs + 2;
        },
        _ => {
            eprintln!("Unsupported upper4={:x}", upper4);
        },
    }
    midi_event
}

fn get_meta_event(data: &Vec<u8>, offset: &mut usize) -> MetaEvent {
    let offs = *offset;
    assert!(data[offs] == 0xff);
    let mut meta_event = MetaEvent::Undef;
    match data[offs + 1] {
        0x01 | 0x04 => {
            *offset = offs + 2; 
            let length = get_variable_length_quantity(data, offset);
            println!("length={}", length);
            let text = get_string(data, offset, length);
            match data[offs + 1] {
                0x01 => { meta_event = MetaEvent::Text(Text {name: text}) },
                0x04 => { meta_event = MetaEvent::InstrumentName(InstrumentName {name: text}) },
                _ => {},
            }
        },
        0x03 => {
            *offset = offs + 2; 
            let length = get_variable_length_quantity(data, offset);
            let text = get_string(data, offset, length);
            let seq_track_name = SequenceTrackName {
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
        0x59 => {
            if data[offs + 2] != 0x02 {
                eprintln!("Unexpected byte {:02x} followeing 0x59 KeySignature meta event",
                    data[offs + 2]);
            }
            let key_signature = KeySignature {
                sf: data[offs + 3] as i16,
                mi: data[offs + 4] != 0,
            };
            meta_event = MetaEvent::KeySignature(key_signature);
            *offset = offs + 5;
        },
        0x7f => {
            *offset = offs + 2;
            let length = get_variable_length_quantity(data, offset);
            let ulength: usize = length as usize;
            let sequencer_event = SequencerEvent {
                data: data[*offset..*offset + ulength].to_vec(),
            };
            meta_event = MetaEvent::SequencerEvent(sequencer_event);
            *offset = *offset + ulength;
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
            let length = get_usize(&data, offset);
            let offset_eot = *offset + length;
            println!("length={}, offset={}, eot={}", length, offset, offset_eot);
            let mut track = Track {
                track_events: Vec::<TrackEvent>::new(),
            };
            let mut got_eot = false;
            while (!got_eot) & (*offset < offset_eot) {
                let track_event = get_track_event(data, offset);
                got_eot = matches!(track_event.event, Event::MetaEvent(MetaEvent::EndOfTrack(_)));
                track.track_events.push(track_event);
            }
            println!("got_eot={}", got_eot);
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
        for w in 0..cmp::min(0x40,(file_size/4) as usize) {
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
