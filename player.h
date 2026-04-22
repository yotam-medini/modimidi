#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include "progress_cb.h"

namespace midi { class Midi; }
class SynthSequencer;

namespace player {

// TODO: PlayerParams -> Params
class PlayerParams {
 public:
  using range_t = std::array<uint8_t, 2>;
  using k2range_t = std::unordered_map<uint8_t, range_t>;
  uint32_t begin_ms_{0};
  uint32_t end_ms_{UINT32_MAX};
  float tempo_div_factor_{1.0};
  int8_t key_shift_{0};
  unsigned tuning_{440};
  k2range_t tracks_velocity_map_;
  k2range_t channels_velocity_map_;
  uint32_t initial_delay_ms_{0};
  uint32_t batch_duration_ms_{0};
  bool interactive_{false};
  progress_callback_t progress_callback_{nullptr};
  uint32_t debug_{0};
};

enum Command { PauseResume, Backward, Forward, Quit };

class Player {
 public:
  class Impl;
  Player(
      const midi::Midi &pm,
      SynthSequencer &ss,
      const PlayerParams &pp);
  ~Player();
  int run();
  void PostCommand(Command);

 private:
  std::unique_ptr<Impl> impl_;
};

}
