#include "util.h"
#include <array>
#include <format>

namespace {

std::array<uint32_t, 3> milliseconds_to_minutes_seconds_millis(uint32_t ms) {
  uint32_t seconds = ms / 1000;
  uint32_t millis = ms % 1000;
  uint32_t minutes = seconds / 60;
  seconds = seconds % 60;
  return {minutes, seconds, millis}; 
}

}; 

std::string milliseconds_to_string(uint32_t ms) {
  const auto msm = milliseconds_to_minutes_seconds_millis(ms);
  std::string s = std::format("{:3d}:{:02d}.{:03d}", msm[0], msm[1], msm[2]);
  return s;
}

std::string milliseconds_to_string_trimmed(uint32_t ms) {
  const auto msm = milliseconds_to_minutes_seconds_millis(ms);
  std::string s = std::format("{:d}:{:02d}.{:03d}", msm[0], msm[1], msm[2]);
  return s;
}

std::string MidiNoteToString(uint8_t n) {
  static constexpr std::array<const char*, 12> dozen_notes =
    {"C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"};
  std::string s;
  if (n >= 12) {
    n -= 12;
    s = std::format("{}{}", dozen_notes[n % 12], n / 12);
  }
  return s;
}
