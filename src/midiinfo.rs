use crate::midi;

pub fn print_midi_info(parsed_midi: &midi::Midi) {
    println!("Midi general information:");
    println!("format={}, {} tracks", parsed_midi.format, parsed_midi.ntrks);
    for (i, ref track) in parsed_midi.tracks.iter().enumerate() {
        print!("track[{}]:", i);
        let mut pc_seen = false;
        let mut pc_last = midi::ProgramChange { channel: 0, program: 0 };
        for track_event in &track.track_events {
            match track_event.event {
                midi::Event::MetaEvent(ref me) => {
		    match me {
			midi::MetaEvent::Text(ref t) => print!(" {}", t.name),
			midi::MetaEvent::Copyright(ref t) => print!(" {}", t.name),
			midi::MetaEvent::SequenceTrackName(ref name) => print!(" {}", name.name),
			midi::MetaEvent::InstrumentName(ref iname) => print!(" {}", iname.name),
			_ => {},
                    };
                },
                midi::Event::MidiEvent(ref me) => {
                    match me {
                        midi::MidiEvent::ProgramChange(pc) => {
                            if (!pc_seen) || (pc_last != *pc) {
                                print!(" (channel={}, program={})", pc.channel, pc.program);
                                pc_seen = true;
                                pc_last = midi::ProgramChange {
                                    channel: pc.channel,
                                    program: pc.program,
                                };
                            }
                        }
			_ => {},
                    };
                },
                _ => {},
            };
        }
        println!("");
    }
}
