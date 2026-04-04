#include "dyntiming.h"
#include <cstdint>
#include <iostream>
#include <format>

void DynamicTiming::SetMicrosecondsPerQuarter(
    uint32_t curr_ticks, uint64_t ms_per_quarter) {
  ms_ref_ = AbsTicksToMs(curr_ticks);
  ticks_ref_ = curr_ticks;
  microseconds_per_quarter_ = ms_per_quarter;
}
uint32_t DynamicTiming::TicksToMs(uint32_t ticks) const {
  uint64_t number = uint64_t{ticks} * microseconds_per_quarter_;
  uint32_t ms = RoundDiv(number, k_ticks_per_quarter_);
  return ms;
}
uint32_t DynamicTiming::AbsTicksToMs(uint32_t abs_ticks) {
  uint64_t numer = uint64_t{abs_ticks - ticks_ref_} *
    microseconds_per_quarter_;
  uint32_t add = RoundDiv(numer, k_ticks_per_quarter_);
  uint32_t ms = ms_ref_ + add;
  return ms;
}

uint32_t DynamicTiming::RoundDiv(uint64_t n, uint64_t d) {
  static const uint64_t u64max32 = std::numeric_limits<uint32_t>::max();
  uint64_t q = (n + d/2) / d;
  if (q > u64max32) {
    std::cerr << std::format("overflow @ RoundDiv({}, {})", n, d);
  }
  uint32_t ret = static_cast<uint32_t>(q);
  return ret;
}
