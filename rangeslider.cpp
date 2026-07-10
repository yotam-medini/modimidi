#include "rangeslider.h"

#include <algorithm>
#include <cstdlib>

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

RangeSlider::RangeSlider(QWidget *parent) : QWidget(parent) {
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void RangeSlider::SetRange(uint32_t minimum, uint32_t maximum) {
  minimum_ = minimum;
  maximum_ = std::max(minimum, maximum);
  low_ = std::clamp(low_, minimum_, maximum_);
  high_ = std::clamp(high_, minimum_, maximum_);
  if (low_ > high_) {
    std::swap(low_, high_);
  }
  update();
}

void RangeSlider::SetValues(uint32_t low, uint32_t high) {
  if (low > high) {
    std::swap(low, high);
  }
  low_ = std::clamp(low, minimum_, maximum_);
  high_ = std::clamp(high, minimum_, maximum_);
  update();
}

void RangeSlider::SetLowValue(uint32_t value) {
  low_ = std::clamp(value, minimum_, high_);
  update();
}

void RangeSlider::SetHighValue(uint32_t value) {
  high_ = std::clamp(value, low_, maximum_);
  update();
}

int RangeSlider::HandleRadius() const {
  // Roughly half a line of text tall, with a sensible floor so it
  // stays grabbable even with tiny fonts.
  return std::max(6, fontMetrics().height() / 2);
}

int RangeSlider::GrooveHeight() const {
  return std::max(2, fontMetrics().height() / 5);
}

int RangeSlider::Margin() const {
  return HandleRadius() + GrooveHeight() / 2;
}

QSize RangeSlider::sizeHint() const {
  const QFontMetrics fm = fontMetrics();
  // Wide enough for a handful of "00:00.000"-ish labels side by side;
  // tall enough for the handles plus a little breathing room.
  const int w = fm.horizontalAdvance(QStringLiteral("00:00.000")) * 6;
  const int h = 2 * HandleRadius() + fm.height() / 2;
  return QSize(w, h);
}

QSize RangeSlider::minimumSizeHint() const {
  const QFontMetrics fm = fontMetrics();
  const int w = fm.horizontalAdvance(QStringLiteral("00:00.000")) * 2;
  const int h = 2 * HandleRadius() + fm.height() / 2;
  return QSize(w, h);
}

int RangeSlider::GrooveLeft() const {
  return Margin();
}

int RangeSlider::GrooveWidth() const {
  return std::max(1, width() - 2 * Margin());
}

int RangeSlider::ValueToX(uint32_t value) const {
  if (maximum_ <= minimum_) {
    return GrooveLeft();
  }
  const double fraction =
    double(value - minimum_) / double(maximum_ - minimum_);
  return GrooveLeft() + int(fraction * GrooveWidth() + 0.5);
}

uint32_t RangeSlider::XToValue(int x) const {
  if (maximum_ <= minimum_) {
    return minimum_;
  }
  const int clamped_x =
    std::clamp(x, GrooveLeft(), GrooveLeft() + GrooveWidth());
  const double fraction =
    double(clamped_x - GrooveLeft()) / double(GrooveWidth());
  return minimum_ + uint32_t(fraction * (maximum_ - minimum_) + 0.5);
}

RangeSlider::Handle RangeSlider::HandleUnderCursor(int x) const {
  const int low_x = ValueToX(low_);
  const int high_x = ValueToX(high_);
  const int radius = HandleRadius();
  const int d_low = std::abs(x - low_x);
  const int d_high = std::abs(x - high_x);
  if (d_low > radius && d_high > radius) {
    return Handle::None;
  }
  // When both handles overlap (or are very close), prefer whichever is
  // closer to the click; ties go to Low so the user can pull them apart.
  return (d_low <= d_high) ? Handle::Low : Handle::High;
}

void RangeSlider::MoveHandle(Handle handle, int x) {
  const uint32_t v = XToValue(x);
  if (handle == Handle::Low) {
    low_ = std::min(v, high_);
  } else if (handle == Handle::High) {
    high_ = std::max(v, low_);
  }
  update();
}

void RangeSlider::mousePressEvent(QMouseEvent *event) {
  if (!isEnabled() || event->button() != Qt::LeftButton ||
      maximum_ <= minimum_) {
    return;
  }
  pressed_handle_ = HandleUnderCursor(event->pos().x());
  if (pressed_handle_ != Handle::None) {
    MoveHandle(pressed_handle_, event->pos().x());
    if (pressed_handle_ == Handle::Low) {
      emit lowValueChanged(low_);
    } else {
      emit highValueChanged(high_);
    }
  }
}

void RangeSlider::mouseMoveEvent(QMouseEvent *event) {
  if (!isEnabled()) {
    return;
  }
  if (pressed_handle_ != Handle::None) {
    MoveHandle(pressed_handle_, event->pos().x());
    if (pressed_handle_ == Handle::Low) {
      emit lowValueChanged(low_);
    } else {
      emit highValueChanged(high_);
    }
  } else {
    const Handle new_hover = HandleUnderCursor(event->pos().x());
    if (new_hover != hover_handle_) {
      hover_handle_ = new_hover;
      update();
    }
    setCursor(
      hover_handle_ != Handle::None ? Qt::PointingHandCursor
                                     : Qt::ArrowCursor);
  }
}

void RangeSlider::mouseReleaseEvent(QMouseEvent *event) {
  Q_UNUSED(event);
  if (pressed_handle_ != Handle::None) {
    pressed_handle_ = Handle::None;
    update();
    emit rangeEdited(low_, high_);
  }
}

void RangeSlider::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const int handle_radius = HandleRadius();
  const int groove_height = GrooveHeight();
  const int mid_y = height() / 2;
  const int groove_left = GrooveLeft();
  const int groove_right = groove_left + GrooveWidth();

  // Background groove (full domain).
  const QRect groove_rect(
    groove_left, mid_y - groove_height / 2,
    groove_right - groove_left, groove_height);
  painter.setPen(Qt::NoPen);
  painter.setBrush(isEnabled() ? QColor(200, 200, 200)
                                : QColor(225, 225, 225));
  painter.drawRoundedRect(groove_rect, groove_height / 2, groove_height / 2);

  const int low_x = ValueToX(low_);
  const int high_x = ValueToX(high_);

  // Highlighted selected sub-range [low_, high_].
  const QRect selected_rect(
    low_x, mid_y - groove_height / 2, high_x - low_x, groove_height);
  painter.setBrush(
    isEnabled() ? QColor(66, 133, 244) : QColor(190, 205, 230));
  painter.drawRoundedRect(
    selected_rect, groove_height / 2, groove_height / 2);

  auto draw_handle = [&](int x, bool active) {
    painter.setPen(QPen(QColor(60, 60, 60), 1));
    painter.setBrush(
      !isEnabled() ? QColor(225, 225, 225)
      : active     ? QColor(30, 100, 220)
                   : QColor(250, 250, 250));
    painter.drawEllipse(QPoint(x, mid_y), handle_radius, handle_radius);
  };
  draw_handle(
    low_x, pressed_handle_ == Handle::Low || hover_handle_ == Handle::Low);
  draw_handle(
    high_x,
    pressed_handle_ == Handle::High || hover_handle_ == Handle::High);
}
