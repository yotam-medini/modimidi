#pragma once

#include <QWidget>

class GPlay;

class Mixer : public QWidget {
 public:
  Mixer(QWidget *page, GPlay &gplay);
  void ResetByMidi();
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
