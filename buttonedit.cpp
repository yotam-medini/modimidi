#include "buttonedit.h"

ButtonEditable::ButtonEditable(
    const std::string &inital_text,
    QWidget *parent) :
  QPushButton(QString::fromStdString(inital_text), parent) {

};
