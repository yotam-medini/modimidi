#pragma once
#include <memory>

class GPlay;

class UI
{
 public:
  UI(int argc, char **argv, GPlay &gplay, bool is_android);
  ~UI();
  int Run();
  void Play();
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
