use std::path::PathBuf;
use std::fs;

pub struct Event {
}

pub struct Track {
    events: Vec<Event>,
}

pub struct Midi {
    error: String,
    format: u8,
    tracks: Vec<Track>,
}

impl Midi {
    fn ok(&self) -> bool {
        self.error.is_empty()
    }
    fn set_error(&mut self, err: String) {
        println!("{}", err);
        self.error = err;
    }
}

fn get_chunk_type(data: &Vec<u8>, offset: usize) -> String {
    let mut chunk_type = String::new();
    for i in offset..offset+4 {
        let cdata: char = char::from_u32(u32::from(data[i])).unwrap();
        chunk_type.push(cdata);
    }
    chunk_type
}

pub fn parse_midi_file(filename: &PathBuf) -> Midi {
    println!("parse_midi_file({:?})", filename);
    let mut midi = Midi {
        error: String::new(),
        format: 0xff,
        tracks: Vec::<Track>::new(),
    };
    let meta = fs::metadata(filename);
    let mut file_size: u64 = 0;
    let mut ok = true;
    match meta {
        Ok(mt) => { println!("mt={:?}", mt); file_size = mt.len(); },
        Err(e) => {
            println!("Error {:?}", e); 
            midi.error = e.to_string();
            ok = false; 
        }
    }
    println!("{:?} size={}", filename, file_size);
    let data: Vec<u8> =
        if ok {fs::read(filename).unwrap() } else { Vec::<u8>::new() };
    if ok {
        // let data: Vec<u8> = fs::read(filename).unwrap();
        println!("#(data)={}", data.len());
        for w in 0..6 {
            println!("header[{:02}]: {:#010b} {:#010b} {:#010b} {:#010b}",
                4*w, data[4*w + 0], data[4*w + 1], data[4*w + 2], data[4*w + 3]);
        }
        let mthd = get_chunk_type(&data, 0);
        println!("mthd={}", mthd);
        if mthd != String::from("MThd") {
            eprintln!("Header chunk: {} != MThd", mthd);
            ok = false;
        }
    }
    if ok {
        let length: u32 = 
            (u32::from(data[4]) << (3*8)) |
            (u32::from(data[5]) << (2*8)) |
            (u32::from(data[6]) << (1*8)) |
            (u32::from(data[7]));
        let midi_format : u16 = (u16::from(data[8]) << 8) | u16::from(data[9]);
        let ntrks : u16 = (u16::from(data[10]) << 8) | u16::from(data[11]);
        println!("length={}, format={}, ntrks={}", length, midi_format, ntrks);
        let division : u16 = (u16::from(data[12]) << 8) | u16::from(data[13]);
        println!("division={:#018b}", division); // division=0b0000000110000000
        let bit15: u16 = division >> 15;
        let mut ticks_per_quarter_note = 0u16;
        let mut negative_smpte_format = 0u8;
        let mut ticks_per_frame = 0u8;
        if bit15 == 0 {
            ticks_per_quarter_note = division;
        } else {
            negative_smpte_format = data[12] & 0x7f;
            ticks_per_frame = data[13];
        }
        println!("ticks_per_quarter_note={}", ticks_per_quarter_note);
        println!("ticks_per_frame={}", ticks_per_frame);
        println!("negative_smpte_format={}", negative_smpte_format);
    }
    return midi;
}
