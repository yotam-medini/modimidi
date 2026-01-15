// -*- c++ -*-
#pragma once

#include <termios.h>
#include <atomic>

class RawTerminal {
 public:
  RawTerminal();
  ~RawTerminal();
  void Restore() noexcept;
 private:
  termios oldt_;
  static void InstallSignalHandlers();
  static void SignalHandler(int sig);
  static std::atomic<RawTerminal*>& ActiveGuard() {
      static std::atomic<RawTerminal*> guard{nullptr};
      return guard;
  }
};
