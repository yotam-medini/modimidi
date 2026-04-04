#pragma once

#include <deque>
#include <string>

class DebugMessage {
 public:
  using messages_t = std::deque<std::string>;
  static void AddMessage(const std::string message);
  static const messages_t &GetMessages();
 private:
  static constexpr size_t CAPACITY = 10;
  static DebugMessage &Create();
  DebugMessage() {}
  messages_t messages_;
};
