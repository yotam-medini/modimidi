// -*- c++ -*-
#pragma once

#include <cstdint>
#include <QWidget>

// A horizontal slider with TWO independently draggable handles ("Low"
// and "High"), for picking a [low, high] sub-range within an overall
// [minimum, maximum] domain — e.g. a start/end time selection.
//
class RangeSlider : public QWidget {
  Q_OBJECT

 public:
  explicit RangeSlider(QWidget *parent = nullptr);

  // Sets the overall [minimum, maximum] domain of the slider.
  // Existing Low/High values are clamped into the new domain.
  void SetRange(uint32_t minimum, uint32_t maximum);

  // Sets both handle values at once (e.g. to reset to full range).
  void SetValues(uint32_t low, uint32_t high);

  // These clamp into the current domain (and against the other
  // handle, same as dragging does) and emit lowValueChanged /
  // highValueChanged, so external editors (e.g. a text-input dialog)
  // stay in sync with the slider the same way a drag would.
  void SetLowHighValue(int i, uint32_t value);
  void SetLowValue(uint32_t value);
  void SetHighValue(uint32_t value);

  // Emits rangeEdited(LowValue(), HighValue()). Callers that changed
  // Low/High via SetLowValue()/SetHighValue() from outside a drag
  // (e.g. a text-input dialog) call this once editing is done, to
  // mirror the "committed edit" signal a mouse release would emit.
  void CommitEdit();

  void SetCurrentPosition(uint32_t ms);


  uint32_t Minimum() const { return minimum_; }
  uint32_t Maximum() const { return maximum_; }
  uint32_t LowHighValue(int i) const { return i == 0 ? low_ : high_; }
  uint32_t LowValue() const { return low_; }
  uint32_t HighValue() const { return high_; }

  QSize minimumSizeHint() const override;

 signals:
  // Emitted continuously while a handle is being dragged.
  void lowValueChanged(uint32_t value);
  void highValueChanged(uint32_t value);

  // Emitted once, when the user releases a handle after moving it:
  // the "committed" edit of the [low, high] sub-range. Not connected
  // to anything by default; the caller can hook it up as needed.
  void rangeEdited(uint32_t low, uint32_t high);

 protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

 private:
  enum class Handle { None, Low, High };

  // Visual metrics, all derived from the widget's current font
  // metrics rather than fixed pixel constants.
  int HandleRadius() const;
  int GrooveHeight() const;
  int Margin() const;

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
};
