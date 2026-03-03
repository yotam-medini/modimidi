#include "util.h"
#include <format>

std::string milliseconds_to_string(uint32_t ms) {
  uint32_t seconds = ms / 1000;
  uint32_t millis = ms % 1000;
  uint32_t minutes = seconds / 60;
  seconds = seconds % 60;
  std::string s = std::format("{:3d}:{:02d}.{:03d}", minutes, seconds, millis);
  return s;
}

