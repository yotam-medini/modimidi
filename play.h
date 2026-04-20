// -*- c++ -*-
#pragma once

#include "player.h"
class SynthSequencer;

extern int Play(
  const midi::Midi &parsed_midi,
  SynthSequencer &synth_sequencer,
  player::PlayerParams &play_params);
