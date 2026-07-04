// -*- c++ -*-
#pragma once

#include <cstdint>
#include <QWidget>

// A horizontal slider with TWO draggable handles ("Low" and "High"),
// used to select a start/end sub-range within [minimum, maximum].
// Intended use here: minimum=0, maximum=<midi file duration in ms>,
// so the user can pick a start/end time to play a sub segment of the
// currently loaded MIDI file.
//
// Pure C++ / QWidget based (no .qml, no .ui/.xml).
//
// Optionally shows a third, non-interactive marker that can track the
// current playback position (see SetCurrentPosition()).
class RangeSlider : public QWidget {
  Q_OBJECT

 public:
  explicit RangeSlider(QWidget *parent = nullptr);

  // Sets the overall [minimum_ms, maximum_ms] domain of the slider.
  // Existing Low/High values are clamped into the new domain.
  void SetRange(uint32_t minimum_ms, uint32_t maximum_ms);

  // Sets both handle values at once (e.g. to reset to full range).
  void SetValues(uint32_t low_ms, uint32_t high_ms);
  void SetLowValue(uint32_t ms);
  void SetHighValue(uint32_t ms);

  // Current playback-position marker (thin line), independent of the
  // two range handles. ClearCurrentPosition() hides it again.
  void SetCurrentPosition(uint32_t ms);
  void ClearCurrentPosition();

  uint32_t Minimum() const { return minimum_; }
  uint32_t Maximum() const { return maximum_; }
  uint32_t LowValue() const { return low_; }
  uint32_t HighValue() const { return high_; }

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

 signals:
  // Emitted continuously while a handle is being dragged.
  void lowValueChanged(uint32_t ms);
  void highValueChanged(uint32_t ms);

  // Emitted once, when the user releases a handle after moving it:
  // the "committed" edit of the [low, high] sub-segment.
  void rangeEdited(uint32_t low_ms, uint32_t high_ms);

 protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

 private:
  enum class Handle { None, Low, High };

  int GrooveLeft() const;
  int GrooveWidth() const;
  int ValueToX(uint32_t value) const;
  uint32_t XToValue(int x) const;
  Handle HandleUnderCursor(int x) const;
  void MoveHandle(Handle handle, int x);

  uint32_t minimum_{0};
  uint32_t maximum_{0};
  uint32_t low_{0};
  uint32_t high_{0};

  bool has_current_position_{false};
  uint32_t current_position_{0};

  Handle pressed_handle_{Handle::None};
  Handle hover_handle_{Handle::None};

  static constexpr int kHandleRadius = 8;
  static constexpr int kGrooveHeight = 4;
  static constexpr int kMargin = kHandleRadius + 2;
};
