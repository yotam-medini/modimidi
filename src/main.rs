use std::path::PathBuf;
// use std::process::ExitCode;
use clap::{Arg, ArgAction, arg, command, value_parser};
mod cfluid;
mod midi;
mod player;
mod seqdemo;
mod sequencer;
mod util;

fn parse_number(s: &str) -> Result<u32, String> {
    let base = if s.starts_with("0x") { 16 } else { 10 };
    u32::from_str_radix(s.trim_start_matches("0x"), base)
        .map_err(|e| format!("Invalid number '{}': {}", s, e))
}

fn parseu32(s: &str, err: &mut String) -> u32 {
   let mut ret: u32 = 0;
   match s.parse::<u32>() {
       Ok(pn) => {ret = pn; err.clear(); },
       Err(perr) => {*err = perr.to_string()},
   }
   ret
}

fn parse_milliseconds(s: &str) -> Result<u32, String> {
    let mut err = String::new();
    let mut parts = s.split(":");
    let ms: Vec<&str> = parts.collect();
    let mslen = ms.len();
    let mut seconds: u32 = 0;
    let mut milli: u32 = 0;
    if mslen > 2 {
        err = format!("{} has {} colon separators", s, mslen-1);
    } else {
        let mut i = 0;
        if mslen == 2 {
            seconds = 60 * parseu32(ms[0], &mut err);
            if !err.is_empty() {
                err = format!("Failed to parse {} reason: {}", ms[0], err);
            }
            i = 1;
        }
        if err.is_empty() {
            parts = ms[i].split(".");
            let sm: Vec<&str> = parts.collect();
            let smlen = sm.len();
            if smlen > 2 {
                err = format!("{} has {} . separators", ms[i], smlen-1);
            } else {
               seconds += parseu32(&sm[0], &mut err);
               if !err.is_empty() {
                   err = format!("Failed to parse {} reason: {}", sm[0], err);
               }
            }
            if err.is_empty() && (smlen == 2) {
                milli = parseu32(&sm[1], &mut err);
                if !err.is_empty() {
                    err = format!("Failed to parse {} reason: {}", sm[1], err);
                }
            }
        }
    }
    if err.is_empty() { Ok(1000*seconds + milli) } else { Err(err) }
}

fn args_get_matches () -> clap::ArgMatches {
    let matches = command!() // requires `cargo` feature
        .version("0.1")
        .author("Yotam Medini Name <yotam.medini@gmail.com>") // unused ?
        .about("Modified Midi Player")
        .arg(arg!(--seqdemo "Run hard-coded sequencer demo"))
        .arg(arg!(-b --begin <TIME> "start time [minutes]:seconds[.millisecs]")
            .required(false)
            .value_parser(parse_milliseconds)
        )
        .arg(arg!(-e --end <TIME> "end time [minutes]:seconds[.millisecs]")
            .required(false)
            .value_parser(parse_milliseconds)
        )
        .arg(arg!(-T --tempo <F> "Speed Multiplier factor]")
            .required(false)
            .default_value("1.0")
            .value_parser(clap::value_parser!(f64)),
        )
        .arg(
            arg!(--debugflags <FLAGS> "Debug bitwise flags")
            .required(false)
            .default_value("0")
            .value_parser(parse_number),
        )
	.arg(
	    arg!(--soundfounts <PATH> "Path to sound fonts file")
            .required(false)
            .default_value("/usr/share/sounds/sf2/FluidR3_GM.sf2")
	)
	.arg(
            Arg::new("delay")
                .long("delay")
                .value_parser(clap::value_parser!(u32))
                .default_value("200") // Default value if not specified
                .help("Initial extra playing delay in milliseconds"),
	)
	.arg(
            Arg::new("batchduration")
                .long("batchduration")
                .value_parser(clap::value_parser!(u32))
                .default_value("10000") // Default value if not specified
                .help("sequencer batch duration in milliseconds"),
	)
	.arg(
            Arg::new("progress")
                .long("progress")
                .action(ArgAction::SetTrue)
                .help("show progress"),
	)
        .arg(arg!([midifile] "The midi file to play")
            .required(true)
            .value_parser(value_parser!(PathBuf))
        )
        .get_matches();
    matches
}

fn main() {
    let matches = args_get_matches();
    let debug_flags: u32 = *(matches.get_one::<u32>("debugflags").unwrap());
    if let Some(seqdemo) = matches.get_one::<bool>("seqdemo") {
        if *seqdemo {
            seqdemo::sequencer();
            return;
        }
    }
    let soundfounts = matches.get_one::<String>("soundfounts").unwrap();
    let batch_duration_ms: u32 = *matches.get_one::<u32>("batchduration").unwrap();
    let initial_delay_ms: u32 = *matches.get_one::<u32>("delay").unwrap();
    let mut sequencer = sequencer::create_sequencer(
	soundfounts, batch_duration_ms, initial_delay_ms);
    let begin: u32 = *matches.get_one::<u32>("begin").unwrap_or(&0);
    let end: u32 = *matches.get_one::<u32>("end").unwrap_or(&0xffffffff);
    let tempo_factor: f64 = *matches.get_one::<f64>("tempo").unwrap();
    let progress : bool = matches.get_flag("progress");
    let midifile = matches.get_one::<PathBuf>("midifile").unwrap();
    let parsed_midi = midi::parse_midi_file(&midifile, debug_flags);
    let exit_code = if parsed_midi.ok() { 0 } else { 1 };
    if parsed_midi.ok() {
        player::play(&mut sequencer, &parsed_midi, begin, end, tempo_factor, progress, debug_flags);
    }
    sequencer::destroy_sequencer(&mut sequencer);
    std::process::exit(exit_code);
}
