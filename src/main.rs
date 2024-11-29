use std::path::PathBuf;
use clap::{arg, command, value_parser};
mod cfluid;
mod sequencer;

fn parse_number(s: &str) -> Result<u32, String> {
    let base = if s.starts_with("0x") { 16 } else { 10 };
    u32::from_str_radix(s.trim_start_matches("0x"), base)
        .map_err(|e| format!("Invalid number '{}': {}", s, e))
}

fn parse_milliseconds(_s: &str) -> Result<u32, String> {
   Ok(13)
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
        .arg(
            arg!(--debugflags <FLAGS> "Debug bitwise flags")
            .required(false)
            .default_value("0")
            .value_parser(parse_number),
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
    if let Some(debug_flags) = matches.get_one::<u32>("debugflags") {
        println!("debug_flags={}", debug_flags);
    }
    if let Some(seqdemo) = matches.get_one::<bool>("seqdemo") {
        println!("seqdemo={}", seqdemo);
        if *seqdemo {
            sequencer::sequencer();
            return;
        }
    }
    let begin: u32 = *matches.get_one::<u32>("begin").unwrap_or(&0);
    println!("begin={}", begin)
}
