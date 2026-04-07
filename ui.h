#pragma once
#include <memory>

class UI
{
 public:
  UI(int argc, char **argv, bool is_android);
  ~UI();
  int Run();
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
