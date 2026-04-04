// -*- c++ -*-
#pragma once

namespace midi { class Midi; }
class SynthSequencer;
namespace player { class PlayerParams; }

extern int Play(
  const midi::Midi &parsed_midi,
  SynthSequencer &synth_sequencer,
  player::PlayerParams &play_params);
