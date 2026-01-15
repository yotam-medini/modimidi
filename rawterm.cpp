#include "rawterm.h"
#include <signal.h>
#include <unistd.h>
#include <initializer_list>

RawTerminal::RawTerminal() {
  const bool is_forground = isatty(STDIN_FILENO) &&
    tcgetpgrp(STDIN_FILENO) == getpgrp();
  if (is_forground) {
    InstallSignalHandlers();
    tcgetattr(STDIN_FILENO, &oldt_);
    termios newt = oldt_;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ActiveGuard().store(this, std::memory_order_release);
  }
}

RawTerminal::~RawTerminal() {
  Restore();
  ActiveGuard().store(nullptr, std::memory_order_release);
}

void RawTerminal::Restore() noexcept {
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt_);
}

void RawTerminal::InstallSignalHandlers() {
  struct sigaction sa {};
  sa.sa_handler = SignalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  for (int sig : {SIGINT, SIGTERM, SIGABRT, SIGQUIT}) {
    sigaction(sig, &sa, nullptr);
  }
}

void RawTerminal::SignalHandler(int sig) {
  if (auto* g = ActiveGuard().load(std::memory_order_acquire)) {
    g->Restore();
  }

  // Re-raise with default action
  signal(sig, SIG_DFL);
  raise(sig);
}
