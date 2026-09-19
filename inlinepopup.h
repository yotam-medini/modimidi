#pragma once

#include <QWidget>

class QEventLoop;
class QMouseEvent;

// A modal-like panel that lives as a plain child widget of a host
// top-level window, instead of opening a separate native window. This
// avoids a touch-coordinate offset bug observed on Android for separate
// windows (confirmed on both a custom popup and a plain
// QDialogButtonBox, so it is not specific to any one widget type). Runs
// identically on every platform -- no platform-specific code.
//
// Usage: build widgets as children of ContentPanel(), then call Exec().
// Exec() blocks (via a nested event loop) until Accept()/Reject() is
// called, or the user taps outside the content panel (-> Reject()).
class InlinePopup : public QWidget {
  Q_OBJECT

 public:
  enum Result { Rejected = 0, Accepted = 1 };

  explicit InlinePopup(QWidget *host);

  QWidget *ContentPanel();
  int Exec();

 public slots:
  void Accept();
  void Reject();

 protected:
  void mousePressEvent(QMouseEvent *event) override;

 private:
  void Finish(int result);

  QWidget *host_;
  QWidget *panel_;
  QEventLoop *loop_ = nullptr;
  int result_ = Rejected;
};
