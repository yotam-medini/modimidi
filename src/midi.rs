use std::cmp;
use std::fmt;
use std::path::PathBuf;
use std::fs;

struct ParseState {
    data: Vec<u8>,
    offset: usize,
    last_status: u8,
    last_channel: u8,
}

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

#[derive(Default)]
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

#[derive(PartialEq, Eq)] 
pub struct ProgramChange { // 0xc
    pub channel: u8,
    pub program: u8,
}
impl fmt::Display for ProgramChange {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "ProgramChange(channel={}, program={})", self.channel, self.program)
    }
}

pub struct PitchWheel { // 0xe
    pub channel: u8,
    pub bend: u16,
}
impl fmt::Display for PitchWheel {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "PitchWheel(channel={}, bend={})", self.channel, self.bend)
    }
}

#[derive(derive_more::Display)]
pub enum MidiEvent {
    NoteOff(NoteOff),
    NoteOn(NoteOn),
    ControlChange(ControlChange),
    ProgramChange(ProgramChange),
    PitchWheel(PitchWheel),
    #[display("Undef")]
    Undef,
}

pub struct SysexEvent {
}

pub struct SequenceNumber { // 0xff 0x01
    number: u16,
}
impl fmt::Display for SequenceNumber {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "SequenceNumber(number={})", self.number)
    }
}

pub struct Text { // 0xff 0x01
    pub name: String,
}
impl fmt::Display for Text {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Text(name={})", self.name)
    }
}

pub struct Copyright { // 0xff 0x02
    pub name: String,
}
impl fmt::Display for Copyright {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Copyright(name={})", self.name)
    }
}

pub struct SequenceTrackName { // 0xff 0x03
    pub name: String,
}
impl fmt::Display for SequenceTrackName {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "SequenceTrackName(name={})", self.name)
    }
}

pub struct InstrumentName  { // 0xff 0x04
    pub name: String,
}
impl fmt::Display for InstrumentName {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "InstrumentName(name={})", self.name)
    }
}

pub struct Lyric  { // 0xff 0x05
    name: String,
}
impl fmt::Display for Lyric {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Lyric(name={})", self.name)
    }
}

pub struct Marker  { // 0xff 0x06
    name: String,
}
impl fmt::Display for Marker {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Marker(name={})", self.name)
    }
}

pub struct Device  { // 0xff 0x09
    name: String,
}
impl fmt::Display for Device {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Device(name={})", self.name)
    }
}

pub struct ChannelPrefix  { // 0xff 0x20
    channel: u8,
}
impl fmt::Display for ChannelPrefix {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "ChannelPrefix(channel={})", self.channel)
    }
}

pub struct Port  { // 0xff 0x21
    port: u8,
}
impl fmt::Display for Port {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Port({})", self.port)
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

pub struct SmpteOffset { // 0xff 0x54
    pub hr: u8,
    pub mn: u8,
    pub se: u8,
    pub fr: u8,
    pub ff: u8,
}
impl fmt::Display for SmpteOffset {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "SmpteOffset(hr={}, mn={}, se={}, fr={}, ff={})",
            self.hr, self.mn, self.se, self.fr, self.ff)
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
    SequenceNumber(SequenceNumber),
    Copyright(Copyright),
    SequenceTrackName(SequenceTrackName),
    InstrumentName(InstrumentName),
    Lyric(Lyric),
    Marker(Marker),
    Device(Device),
    ChannelPrefix(ChannelPrefix),
    Port(Port),
    EndOfTrack(EndOfTrack),
    SetTempo(SetTempo),
    SmpteOffset(SmpteOffset),
    TimeSignature(TimeSignature),
    KeySignature(KeySignature),
    SequencerEvent(SequencerEvent),
    Undef,
}
impl fmt::Display for MetaEvent {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            MetaEvent::Text(t) => write!(f, "{}", t),
            MetaEvent::SequenceNumber(sn) => write!(f, "{}", sn),
            MetaEvent::Copyright(t) => write!(f, "{}", t),
            MetaEvent::SequenceTrackName(name) => write!(f, "{}", name),
            MetaEvent::InstrumentName(iname) => write!(f, "{}", iname),
            MetaEvent::Lyric(lyric) => write!(f, "{}", lyric),
            MetaEvent::Marker(marker) => write!(f, "{}", marker),
            MetaEvent::Device(device) => write!(f, "{}", device),
            MetaEvent::ChannelPrefix(cp) => write!(f, "{}", cp),
            MetaEvent::Port(port) => write!(f, "{}", port),
            MetaEvent::EndOfTrack(_eot) => write!(f, "EndOfTrack"),
            MetaEvent::SetTempo(st) => write!(f, "{}", st),
            MetaEvent::SmpteOffset(so) => write!(f, "{}", so),
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
        write!(f, "TrackEvent(delta_time={}, event={})", self.delta_time, self.event)
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
    pub format: u16,
    pub ntrks: u16,
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

fn get_usize(state: &mut ParseState) -> usize {
    let offs: usize = state.offset;
    let ret: usize = 
        (usize::from(state.data[offs + 0]) << (3*8)) |
        (usize::from(state.data[offs + 1]) << (2*8)) |
        (usize::from(state.data[offs + 2]) << (1*8)) |
        (usize::from(state.data[offs + 3]));
    state.offset = offs + 4;
    ret
}

fn get_chunk_type(state: &mut ParseState) -> String {
    let mut chunk_type = String::new();
    let next_offset: usize = state.offset + 4;
    for i in state.offset..next_offset {
        let cdata: char = char::from_u32(u32::from(state.data[i])).unwrap();
        chunk_type.push(cdata);
    }
    state.offset = next_offset;
    chunk_type
}

fn get_variable_length_quantity(state: &mut ParseState) -> u32 {
    let mut quantity: u32 = 0;
    let mut offs: usize = state.offset;
    let mut done = false;
    let offs_limit = offs + 4;
    while (offs < offs_limit) && !done {
        let b: u8 = state.data[offs];
        quantity = (quantity << 7) + (u32::from(b) & 0x7f);
        done = (b & 0x80) == 0;
        offs += 1;
    }
    state.offset = offs;
    quantity
}

fn get_sized_quantity(state: &mut ParseState) -> u32 {
    let offs: usize = state.offset + 2;
    let n_bytes = state.data[offs] as usize;
    let mut quantity: u32 = 0;
    for i in offs+1..offs+1+n_bytes {
        quantity = (quantity << 8) + u32::from(state.data[i]);
    }
    state.offset = offs + 1 + n_bytes;
    quantity
}

fn get_string(state: &mut ParseState, length: u32) -> String {
    let mut text = String::new();
    let next_offset: usize = state.offset + (length as usize);
    for i in state.offset..next_offset {
        let cdata: char = char::from_u32(u32::from(state.data[i])).unwrap();
        text.push(cdata);
    }
    state.offset = next_offset;
    text
}

fn get_track_event(state: &mut ParseState) -> TrackEvent {
    let delta_time = get_variable_length_quantity(state);
    let event_first_byte = state.data[state.offset];
    let mut te = TrackEvent {
        delta_time: delta_time,
        event: Event::Undef,
    };
    match event_first_byte {
        0xff => { // Meta Event
            let meta_event = get_meta_event(state);
            te.event = Event::MetaEvent(meta_event);
        },
        0xf0 | 0xf7 => { // Sysex Event
            println!("Sysex Event ignored")
        },
        _ => { // Midi Event
            let midi_event = get_midi_event(state);
            te.event = Event::MidiEvent(midi_event);
        }
    }
    te
}

fn get_midi_event(state: &mut ParseState) -> MidiEvent {
    let mut offs = state.offset;
    let mut midi_event = MidiEvent::Undef;
    let upper4 = (state.data[offs] >> 4) & 0xff;
    if upper4 & 0x8 != 0 {
        state.last_status = upper4 & 0x7;
        state.last_channel = state.data[offs] & 0xf;
        offs += 1;
    }
    match state.last_status {
        0x0 => {
            let note_off = NoteOff {
                channel: state.last_channel,
                key: state.data[offs],
                velocity: state.data[offs + 1],
            };
            midi_event = MidiEvent::NoteOff(note_off);
            state.offset = offs + 2;
        },
        0x1 => {
            let note_on = NoteOn {
                channel: state.last_channel,
                key: state.data[offs],
                velocity: state.data[offs + 1],
            };
            midi_event = MidiEvent::NoteOn(note_on);
            state.offset = offs + 2;
        },
        // 0xa =>  Key Pressure
        0x3 => {
            let cc = ControlChange{
                channel: state.last_channel,
                number: state.data[offs],
                value: state.data[offs + 1],
            };
            midi_event = MidiEvent::ControlChange(cc);
            state.offset = offs + 2;
        },
        0x4 => {
            let pc = ProgramChange{channel: state.last_channel, program: state.data[offs]};
            midi_event = MidiEvent::ProgramChange(pc);
            state.offset = offs + 1;
        },
        0x6 => {
            let lllllll: u16 = (state.data[offs] & 0x7f) as u16;
            let mmmmmmm: u16 = (state.data[offs + 1] & 0x7f) as u16;
            let bend: u16 = (mmmmmmm << 7) | lllllll;
            let pw = PitchWheel{channel: state.last_channel, bend: bend, };
            midi_event = MidiEvent::PitchWheel(pw);
            state.offset = offs + 2;
        },
        _ => {
            eprintln!("Unsupported upper4={:x} last_status={}, data[{}]={:x}",
		upper4, state.last_status, offs, state.data[offs]);
        },
    }
    midi_event
}

fn get_meta_event(state: &mut ParseState) -> MetaEvent {
    let offs = state.offset;
    // let data = &state.data;
    assert!(state.data[offs] == 0xff);
    let mut meta_event = MetaEvent::Undef;
    match state.data[offs + 1] {
        0x00 => {
            let length = state.data[offs + 2] as usize;
            if length != 2 {
                eprintln!("Unexpected length={}!=2 in SequenceNumber", length);
            }
            meta_event = MetaEvent::SequenceNumber(SequenceNumber { number:
                ((state.data[offs + 3] as u16) << 8) |
               (state.data[offs + 3] as u16)
            });
           state.offset = offs + 3 + length;
        },
        0x01 | 0x02 | 0x03 | 0x04 | 0x05 | 0x06 | 0x09 => {
            state.offset = offs + 2; 
            let length = get_variable_length_quantity(state);
            let text = get_string(state, length);
            match state.data[offs + 1] {
                0x01 => { meta_event = MetaEvent::Text(Text {name: text}) },
                0x02 => { meta_event = MetaEvent::Copyright(Copyright {name: text}) },
                0x03 => { meta_event = MetaEvent::SequenceTrackName(
                    SequenceTrackName {name: text}) },
                0x04 => { meta_event = MetaEvent::InstrumentName(InstrumentName {name: text}) },
                0x05 => { meta_event = MetaEvent::Lyric(Lyric {name: text}) },
                0x06 => { meta_event = MetaEvent::Marker(Marker {name: text}) },
                0x09 => { meta_event = MetaEvent::Device(Device {name: text}) }, // non-standard
                _ => {},
            }
        },
        0x20 => {
            state.offset = offs + 2; 
            let length = get_variable_length_quantity(state);
            if length != 1 {
                eprintln!("Unexpected length={}!=1 in ChannelPrefix", length);
            }
            meta_event = MetaEvent::ChannelPrefix(ChannelPrefix { 
                channel: state.data[state.offset], 
            });
            state.offset = state.offset + (length as usize);
        },
        0x21 => {
            state.offset = offs + 2; 
            let length = get_variable_length_quantity(state);
            if length != 1 {
                eprintln!("Unexpected length={}!=1 in Port", length);
            }
            meta_event = MetaEvent::Port(Port { port: state.data[state.offset], });
            state.offset = state.offset + (length as usize);
        },
        0x2f => {
            let n_bytes: usize = usize::from(state.data[offs + 2]);
            if n_bytes != 0 {
                eprintln!("Unexpected n_bytes={} in EndOfTrack", n_bytes);
            }
            meta_event = MetaEvent::EndOfTrack(EndOfTrack {});
            state.offset = offs + 3 + n_bytes;
        },
        0x51 => {
            let set_tempo = SetTempo { tttttt: get_sized_quantity(state), };
            meta_event = MetaEvent::SetTempo(set_tempo);
        },
        0x54 => {
            let n_bytes: usize = usize::from(state.data[offs + 2]);
            if n_bytes != 5 {
                eprintln!("Unexpected n_bytes={}!=5 in SMPTE Offset", n_bytes);
            }
            let smpte_offset = SmpteOffset {
                hr: state.data[offs + 3 + 0],
                mn: state.data[offs + 3 + 1],
                se: state.data[offs + 3 + 2],
                fr: state.data[offs + 3 + 3],
                ff: state.data[offs + 3 + 4],
            };
            meta_event = MetaEvent::SmpteOffset(smpte_offset);
            state.offset = offs + 3 + n_bytes;
        },
        0x58 => {
            if state.data[offs + 2] != 0x04 {
                eprintln!("Unexpected byte {:02x} followeing 0x58 TimeSignature meta event",
                    state.data[offs + 2]);
            }
            let time_signature = TimeSignature {
                nn: state.data[offs + 3],
                dd: state.data[offs + 4],
                cc: state.data[offs + 5],
                bb: state.data[offs + 6]
            };
            meta_event = MetaEvent::TimeSignature(time_signature);
            state.offset = offs + 7;
        },
        0x59 => {
            if state.data[offs + 2] != 0x02 {
                eprintln!("Unexpected byte {:02x} followeing 0x59 KeySignature meta event",
                    state.data[offs + 2]);
            }
            let key_signature = KeySignature {
                sf: state.data[offs + 3] as i16,
                mi: state.data[offs + 4] != 0,
            };
            meta_event = MetaEvent::KeySignature(key_signature);
            state.offset = offs + 5;
        },
        0x7f => {
            state.offset = offs + 2;
            let length = get_variable_length_quantity(state);
            let ulength: usize = length as usize;
            let sequencer_event = SequencerEvent {
                data: state.data[state.offset..state.offset + ulength].to_vec(),
            };
            meta_event = MetaEvent::SequencerEvent(sequencer_event);
            state.offset = state.offset + ulength;
        },
        _ => {
            state.offset = offs + 2; 
            let length = get_variable_length_quantity(state);
            eprintln!("Unsupported MetaEvent {:#02x} length={} at offset={}",
                state.data[offs + 1], length, offs);
            state.offset += length as usize;
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
    fn read_one_track(&mut self, state: &mut ParseState) {
        self.read_track(state);
    }
    fn read_tracks(&mut self, state: &mut ParseState) {
        for _itrack in 0..self.ntrks {
            if self.ok() {
                self.read_track(state);
            }
        }
    }
    fn read_track(&mut self, state: &mut ParseState) {
        const MTRK: &str = "MTrk";
        let chunk_type = get_chunk_type(state);
        if chunk_type != MTRK {
            self.set_error(format!("chunk_type={} != {} @ offset={}",
                chunk_type, MTRK, state.offset));
        } else {
            let length = get_usize(state);
            let offset_eot = state.offset + length;
            let mut track = Track {
                track_events: Vec::<TrackEvent>::new(),
            };
            let mut got_eot = false;
            while (!got_eot) & (state.offset < offset_eot) {
                let track_event = get_track_event(state);
                got_eot = matches!(track_event.event, Event::MetaEvent(MetaEvent::EndOfTrack(_)));
                track.track_events.push(track_event);
            }
            self.tracks.push(track);
            state.offset = offset_eot;
        }
    }
}

fn dump_start(data: &Vec<u8>) {
    println!("#(data)={}", data.len());
    for w in 0..cmp::min(0x40, (data.len()/4) as usize) {
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
}

pub fn parse_midi_file(filename: &PathBuf, debug_flags: u32) -> Midi {
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
    match meta {
        Ok(mt) => { 
            let file_size = mt.len(); 
            if debug_flags & 0x2 != 0 { println!("{:?} file_size={}", filename, file_size); };
        },
        Err(e) => {
            eprintln!("Error {:?}", e); 
            midi.set_error(format!("Error {:?}", e));
        }
    }
    let data: Vec<u8> =
        if midi.ok() {fs::read(filename).unwrap() } else { Vec::<u8>::new() };
    let mut parse_state = ParseState {
        data: data,
        offset: 0,
        last_status: 0,
        last_channel: 0,
    };
    if midi.ok() {
        if debug_flags & 0x1 != 0 { dump_start(&parse_state.data); }
        const MTHD: &str = "MThd";
        let mthd = get_chunk_type(&mut parse_state);
        if mthd != MTHD {
            midi.set_error(format!("Header chunk: {} != {}", mthd, MTHD));
        }
    }
    if midi.ok() {
	parse_state.offset = 4;
        length = get_usize(&mut parse_state);
        midi.format = (u16::from(parse_state.data[8]) << 8) | u16::from(parse_state.data[9]);
        midi.ntrks = (u16::from(parse_state.data[10]) << 8) | u16::from(parse_state.data[11]);
        if debug_flags & 0x2 != 0 {
            println!("length={}, format={}, ntrks={}", length, midi.format, midi.ntrks);
        }
        if length != 6 {
            eprintln!("Unexpected length: {} != 6", length);
        }
        let division : u16 = (u16::from(parse_state.data[12]) << 8) | u16::from(parse_state.data[13]);
        if debug_flags & 0x2 != 0 {
            println!("division={:#018b}", division); // division=0b0000000110000000
        }
        let bit15: u16 = division >> 15;
        if bit15 == 0 {
            midi.ticks_per_quarter_note = division;
        } else {
            midi.negative_smpte_format = parse_state.data[12] & 0x7f;
            midi.ticks_per_frame = parse_state.data[13];
            // hack
            midi.ticks_per_quarter_note =
                (0x100u16 - (parse_state.data[12] as u16)) // negative two's compliment
                * (parse_state.data[13] as u16);
        }
        if debug_flags & 0x2 != 0 {
            println!("ticks_per_quarter_note={}", midi.ticks_per_quarter_note);
            println!("ticks_per_frame={}", midi.ticks_per_frame);
            println!("negative_smpte_format={}", midi.negative_smpte_format);
        }
    }
    if midi.ok() {
        parse_state.offset = 4 + 4 + length;
        match midi.format {
            0 => midi.read_one_track(&mut parse_state),
            1|2 => midi.read_tracks(&mut parse_state),
            _ => midi.set_error(format!("Unsupported midi format: {}",
                midi.format))
        }
    }
    return midi;
}
