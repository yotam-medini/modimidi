#pragma once

#include <string>
#include <QPushButton>

class QWidget;

class ButtonEditable : public QPushButton {
 public:
  ButtonEditable(
    const std::string &inital_text,
    QWidget *parent);
};
