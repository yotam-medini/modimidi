#pragma once

#include <QComboBox>

// QComboBox subclass whose dropdown is a plain child widget of the
// top-level window, instead of a separate native popup window. This
// avoids a touch-coordinate offset bug observed on Android for secondary
// windows (confirmed on both a custom popup and a plain
// QDialogButtonBox, so it is not specific to any one widget type).
// Runs the same way on every platform -- no platform-specific branches.
class TouchComboBox : public QComboBox {
  Q_OBJECT

 public:
  using QComboBox::QComboBox;

 protected:
  void showPopup() override;
};
