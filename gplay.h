// -*- c++ -*-
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "progress_cb.h"

class GPlay {
 public:
  GPlay(bool is_android);
  ~GPlay();
  std::string OpenMidi(std::vector<uint8_t> data); // return error
  void SetProgressCallback(progress_callback_t progress_cb);
  void Play();
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
