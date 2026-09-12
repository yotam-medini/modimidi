#pragma once

#include <QComboBox>

// QComboBox subclass since on Android,
// Qt's doesn't forward touch-release event into that popup correctly.
class TouchComboBox : public QComboBox {
  Q_OBJECT

 public:
  using QComboBox::QComboBox;

 protected:
  void showPopup() override;
};
