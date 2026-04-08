// -*- c++ -*-
#pragma once

#include <memory>
#include <string>

class GPlay {
 public:
  GPlay();
  ~GPlay();
  std::string OpenMidi(const std::string &path); // return error
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
