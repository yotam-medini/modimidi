use std::path::PathBuf;
use clap::{arg, command, value_parser};
mod cfluid;
mod sequencer;

fn parse_number(s: &str) -> Result<u32, String> {
    let base = if s.starts_with("0x") { 16 } else { 10 };
    u32::from_str_radix(s.trim_start_matches("0x"), base)
        .map_err(|e| format!("Invalid number '{}': {}", s, e))
}

fn args_get_matches () -> clap::ArgMatches {
    let matches = command!() // requires `cargo` feature
        .version("0.1")
        .author("Yotam Medini Name <yotam.medini@gmail.com>") // unused ?
        .about("Modified Midi Player")
        .arg(arg!([name] "Optional name to operate on")
             .required(true)
        )
        .arg(arg!(--seqdemo "Run hard-coded sequencer demo"))
        .arg(
            arg!(--debugflags <FLAGS> "Debug bitwise flags")
            .required(false)
            .default_value("0")
            .value_parser(parse_number),
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
        }
    }
}
