// -*- c++ -*-
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "progress_cb.h"
#include "state.h"

namespace midi {
class Midi;
} // namespace midi

class GPlay {
 public:
  GPlay(bool is_android);
  ~GPlay();
  std::string OpenMidi(std::vector<uint8_t> data); // return error
  uint32_t GetMidiTotalMilliSeconds() const;
  void SetProgressCallback(progress_callback_t progress_cb);
  void SetStateCallback(OnStateChange_t on_state_change);
  void Play();
  void Stop();
  void PauseResume();
  void SkipForward();
  void SkipBackward();
  State GetState() const;
  void SetBeginEnd(int i, uint32_t ms);
  uint32_t GetBeginEnd(int i) const {
    return i == 0 ? GetBegin() : GetEnd();
  }
  void SetBegin(uint32_t ms);
  uint32_t GetBegin() const;
  void SetEnd(uint32_t ms);
  uint32_t GetEnd() const;
  void SetTempoFactor(float x);
  float GetTempoFactor() const;
  void SetKeyShift(int8_t ks);

  const midi::Midi *GetMidi() const;
  std::string GetMidiInfo() const;

 private:
  State state_{State::None};
  class Impl;
  std::unique_ptr<Impl> impl_;
};
