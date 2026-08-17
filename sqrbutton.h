#pragma once
#include <QToolButton>

class SquareToolButton : public QToolButton {
public:
  SquareToolButton(
    QWidget *parent = nullptr,
    unsigned frac_num = 1,
    unsigned frac_denom = 4);
  QSize sizeHint() const override;

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  unsigned frac_num_;
  unsigned frac_denom_;
};
