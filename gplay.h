// -*- c++ -*-
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class GPlay {
 public:
  GPlay();
  ~GPlay();
  std::string OpenMidi(std::vector<uint8_t> data); // return error
  void Play();
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
