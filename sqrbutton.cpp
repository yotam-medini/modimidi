#include "sqrbutton.h"
#include <algorithm>
#include <QResizeEvent>
#include "qutil.h"

SquareToolButton::SquareToolButton(
    QWidget *parent,
    unsigned frac_num,
    unsigned frac_denom) :
    QToolButton(parent),
    frac_num_{frac_num},
    frac_denom_{frac_denom} {
}

QSize SquareToolButton::sizeHint() const {
  QWidget *parent = parentWidget();
  QSize size_hint;
  if (parent) {
    int parent_size = std::min(parent->width(), parent->height());
    int size = static_cast<int>((parent_size * frac_num_)/frac_denom_);
    size = std::max(size, 24);
    size_hint = QSize(size, size);
  } else {
    size_hint = QToolButton::sizeHint();
  }
  return size_hint;
}

#if 0
void SquareToolButton::resizeEvent(QResizeEvent *event) {
  QToolButton::resizeEvent(event);
}
#endif
