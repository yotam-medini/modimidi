// -*- c++ -*-
#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>
#include "midi.h"

class PlayParams {
 public:
  using range_t = std::array<uint8_t, 2>;
  using k2range_t = std::unordered_map<uint8_t, range_t>;
  uint32_t begin_ms_{0};
  uint32_t end_ms_{0};
  float tempo_div_factor_{1.0};
  int8_t key_shift_{0};
  unsigned tuning_{440};
  k2range_t tracks_velocity_map_;
  k2range_t channels_velocity_map_;
  uint32_t initial_delay_ms_{0};
  uint32_t batch_duration_ms_{0};
  bool progress_{false};
  uint32_t debug_{0};
};

class SynthSequencer;
extern int Play(
  const midi::Midi &parsed_midi,
  SynthSequencer &synth_sequencer,
  const PlayParams &play_params);
