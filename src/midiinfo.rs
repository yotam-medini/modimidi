use crate::midi;

pub fn print_midi_info(parsed_midi: &midi::Midi) {
    println!("Midi general information:");
    println!("format={}, {} tracks", parsed_midi.format, parsed_midi.ntrks);
}
