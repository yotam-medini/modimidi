use std::path::PathBuf;
use std::fs;

pub struct MidiEvent {
}

pub struct SysexEvent {
}

pub struct Text {
    name: String,
}

pub struct SequenceTrackName {
    name: String,
}

pub enum MetaEvent {
    Text(Text),
    SequenceTrackName(SequenceTrackName),
}

pub enum Event {
    MidiEvent,
    SysexEvent,
    MetaEvent(MetaEvent),
    Undef,
}

pub struct TrackEvent {
    delta_time: u32,
    event: Event,
}

pub struct Track {
    track_events: Vec<TrackEvent>,
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
        quantity = (quantity << 7) & (u32::from(data[offs]) & 0x7f);
        done = (data[offs] & 0x80) == 0;
        offs += 1;
    }
    *offset = offs;
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
    if event_first_byte == 0xff { // MetaEvent
        println!("meta... {:#02x} {:#02x}", data[*offset + 1], data[*offset + 2]);
        let meta_event = get_meta_event(data, offset);
        println!("offset={}", offset);
        te.event = Event::MetaEvent(meta_event);
    }
    te
}

fn get_meta_event(data: &Vec<u8>, offset: &mut usize) -> MetaEvent {
    let offs = *offset;
    assert!(data[offs] == 0xff);
    let mut seq_track_name = SequenceTrackName {
        name: String::new(),
    };
    let mut meta_event = MetaEvent::SequenceTrackName(seq_track_name);
    match data[offs + 1] {
        0x01 => {
            *offset = offs + 2; 
            let length = get_variable_length_quantity(data, offset);
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
            println!("itrack={}", itrack);
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
            let offset_eot = offset_begin_of_track + length;
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
            *offset = offset_begin_of_track + length;
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
        for w in 0..0x10 {
            println!("header[{:02}]: {:#010b} {:#010b} {:#010b} {:#010b}",
                4*w, data[4*w + 0], data[4*w + 1], data[4*w + 2], data[4*w + 3]);
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
