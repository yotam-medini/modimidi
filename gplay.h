// -*- c++ -*-
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "progress_cb.h"
#include "state.h"

class GPlay {
 public:
  GPlay(bool is_android);
  ~GPlay();
  std::string OpenMidi(std::vector<uint8_t> data); // return error
  void SetProgressCallback(progress_callback_t progress_cb);
  void SetStateCallback(OnStateChange_t on_state_change);
  void Play();
  void Stop();
  void PauseResume();
  void SkipForward();
  void SkipBackward();
  State GetState() const;
 private:
  State state_{State::None};
  class Impl;
  std::unique_ptr<Impl> impl_;
};
