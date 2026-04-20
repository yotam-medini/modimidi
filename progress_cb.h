// -*- c++ -*-
#pragma once

#include <cstdint>
#include <functional>
#include <string>

using progress_callback_t = std::function<void(
  uint32_t done_ms,
  uint32_t final_ms,
  const std::string&,
  const std::string&)>;
