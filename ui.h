#pragma once
#include <memory>

class UI
{
 public:
  UI(int argc, char **argv, bool is_android);
  ~UI();
  int Run();
 private:
  class UiImpl;
  std::unique_ptr<UiImpl> impl_;
};
