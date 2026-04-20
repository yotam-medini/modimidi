#include "play.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <thread>
#include <unordered_map>

#include <poll.h>
#include <sys/eventfd.h>

#include "player.h"
#include "rawterm.h"

namespace { // private

enum class KeyAction { None, PauseResume, Backward, Forward, Quit, Help };

KeyAction KeyboardProbe() {
  KeyAction action = KeyAction::None;
  char key_char;
  if (read(STDIN_FILENO, &key_char, 1) == 1) {
    switch (key_char) {
     case ' ':
      action = KeyAction::PauseResume;
      break;
     case 'h':
      action = KeyAction::Help;
      break;
     case 'j':
      action = KeyAction::Backward;
      break;
     case 'k':
      action = KeyAction::Forward;
      break;
     case 'q':
      action = KeyAction::Quit;
      break;
     case '\x1b': // Esc
      {
        char cseq[2];
        if ((read(STDIN_FILENO, &cseq[0], 2) == 2) && (cseq[0] == '[') &&
            ((cseq[1] == 'C') || (cseq[1] == 'D'))) {
          action = (cseq[1] == 'C') ? KeyAction::Forward : KeyAction::Backward;
        }
      }
      break;
     default:
      action = KeyAction::None;
    }
  }
  if (action != KeyAction::None) {
    tcflush(STDIN_FILENO, TCIFLUSH);
  }
  return action;
}

void KeyboardHelp() {
  std::cerr << R"(
    modimidi supports the following keyboard commands:
    SPACE:          Pause or Resume
    j, Left-Arrow:  Skip back 5 seconds
    k, Right-Arrow: Skip forward 5 seconds
    q:              Quit
    h:              Show this help message
  )";
}

void handle_keyboard(player::Player& p) {
  static const auto ka2cmd = std::unordered_map<KeyAction, player::Command>{
    {KeyAction::PauseResume, player::Command::PauseResume},
    {KeyAction::Backward, player::Command::Backward},
    {KeyAction::Forward, player::Command::Forward},
    {KeyAction::Quit, player::Command::Quit}
  };
  auto action = KeyboardProbe();
  auto iter = ka2cmd.find(action);
  if (iter != ka2cmd.end()) {
    p.PostCommand(iter->second);
  } else {
    switch (action) {
     case KeyAction::None:
      break;
     case KeyAction::Help:
      KeyboardHelp();
      break;
     default:
      std::cerr << std::format("{}:{} Unexpected action={}\n",
        __FILE__, __LINE__, static_cast<int>(action));
    }
  }
}

} // private namespace 

int Play(
    const midi::Midi &parsed_midi,
    SynthSequencer &synth_sequencer,
    player::PlayerParams &play_params) {
  RawTerminal raw_terminal;
  std::cout << std::format("IsForground={}\n", raw_terminal.IsForground());
  bool interact = raw_terminal.IsForground() && play_params.interactive_;
  int fd_end_of_thread = -1;
  if (interact) {
    fd_end_of_thread = eventfd(0, EFD_NONBLOCK);
    if (fd_end_of_thread < 0) {
      std::cerr << "eventfd failed, canceling interaction\n";
      interact = false;
    }
  }
  if (interact) {
    play_params.progress_callback_ =
      [](uint32_t, uint32_t,
        const std::string& mmss_done, const std::string& mmss_final) {
          std::cout << std::format("\rProgress: {} / {}",
            mmss_done, mmss_final);
        };
  }
  auto p = player::Player(parsed_midi, synth_sequencer, play_params);
  int rc = 0;
  if (interact) {
    std::jthread jth([&rc, &p, fd_end_of_thread]() { 
      rc = p.run();
      char somechar = 42;
      uint64_t nonzero_value = 1;
      write(fd_end_of_thread, &nonzero_value, sizeof(nonzero_value));
      std::cerr << "Wrote to fd_end_of_thread\n";
    });
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = fd_end_of_thread;
    fds[1].events = POLLIN;
    uint64_t eot_val = 0;
    while ((eot_val == 0) && (jth.get_id() != std::thread::id())) {
      auto poll_ret = poll(fds, 2, -1);
      std::cerr << std::format("\npoll_ret={}\n", poll_ret);
      if (fds[1].revents & POLLIN) {
        ssize_t bytes_read = read(fd_end_of_thread, &eot_val, sizeof(eot_val));
        if (bytes_read != sizeof(eot_val)) {
          std::cerr << std::format("Unexpected bytes_read={} != {}\n",
            bytes_read, sizeof(eot_val));
        } else {
          std::cerr << std::format("eot_val=={}\n", eot_val);
        }
      }
      if (fds[0].revents & POLLIN) {
        handle_keyboard(p);
      }
    }
    close(fd_end_of_thread);
  } else {
    rc = p.run();
  }
  if (play_params.interactive_) { std::cout << '\n'; }
  return rc;
}
