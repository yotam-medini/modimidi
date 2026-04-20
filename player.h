#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace midi { class Midi; }
class SynthSequencer;

namespace player {

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
  bool progress_{false};
  uint32_t debug_{0};
};

class Player {
 public:
  class Impl;
  Player(
      const midi::Midi &pm,
      SynthSequencer &ss,
      const PlayerParams &pp,
      bool sense_keyboard);
  ~Player();
  int run();

 private:
  std::unique_ptr<Impl> impl_;
};

}
