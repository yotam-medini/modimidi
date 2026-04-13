#include "debug.h"

DebugMessage &DebugMessage::Create() {
  static DebugMessage single;
  return single;
}

void DebugMessage::AddMessage(const std::string message) {
  DebugMessage &single = Create();
  while (single.messages_.size() >= CAPACITY) {
    single.messages_.pop_front();
  }
  single.messages_.push_back(message);
}

auto DebugMessage::GetMessages() -> const messages_t& {
  DebugMessage &single = Create();
  return single.messages_;
}
