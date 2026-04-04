#pragma once

#include <cstdint>

class DynamicTiming {
 public:
  DynamicTiming(
    uint64_t microseconds_per_quarter=0,
    uint64_t k_ticks_per_quarter=0,
    uint32_t ticks_ref=0,
    uint32_t ms_ref=0) :
      microseconds_per_quarter_{microseconds_per_quarter},
      k_ticks_per_quarter_{k_ticks_per_quarter},
      ticks_ref_{ticks_ref},
      ms_ref_{ms_ref} {
  }
  void SetMicrosecondsPerQuarter(uint32_t curr_ticks, uint64_t ms_per_quarter);
  uint32_t TicksToMs(uint32_t ticks) const;
  uint32_t AbsTicksToMs(uint32_t abs_ticks);

 private:
  static uint32_t RoundDiv(uint64_t n, uint64_t d);
  uint64_t microseconds_per_quarter_{0};
  uint64_t k_ticks_per_quarter_{0};
  uint32_t ticks_ref_{0};
  uint32_t ms_ref_{0};
};

