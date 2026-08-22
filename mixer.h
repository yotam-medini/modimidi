#pragma once

#include <QWidget>

class GPlay;

class Mixer : public QWidget {
 public:
  Mixer(QWidget *page, GPlay &gplay);
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
