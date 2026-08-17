#include "sqrbutton.h"
#include <algorithm>
#include <QResizeEvent>

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
  if (!parent) return QToolButton::sizeHint();

  int parent_size = std::min(parent->width(), parent->height());
  int size = static_cast<int>((parent_size * frac_num_)/frac_denom_);
  size = std::max(size, 24);
  return QSize(size, size);
}

void SquareToolButton::resizeEvent(QResizeEvent *event) {
  int size = std::min(event->size().width(), event->size().height());
  
  if (width() != size || height() != size) {
    blockSignals(true);
    setFixedSize(size, size);
    blockSignals(false);
  }
  QToolButton::resizeEvent(event);
}
