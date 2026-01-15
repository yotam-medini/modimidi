// -*- c++ -*-
#pragma once

#include <termios.h>
#include <atomic>

class RawTerminal {
 public:
  RawTerminal();
  ~RawTerminal();
  void Restore() noexcept;
  bool IsForground() const { return is_forground_; }
 private:
  bool is_forground_{false};
  termios oldt_;
  static void InstallSignalHandlers();
  static void SignalHandler(int sig);
  static std::atomic<RawTerminal*>& ActiveGuard() {
      static std::atomic<RawTerminal*> guard{nullptr};
      return guard;
  }
};
