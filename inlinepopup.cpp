#include "inlinepopup.h"

#include <QEventLoop>
#include <QFrame>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPalette>
#include <QVBoxLayout>

InlinePopup::InlinePopup(QWidget *host) : QWidget{host}, host_{host} {
  setAutoFillBackground(true);
  QPalette backdrop_palette = palette();
  backdrop_palette.setColor(QPalette::Window, QColor(0, 0, 0, 140));
  setPalette(backdrop_palette);

  auto panel_frame = new QFrame(this);
  panel_frame->setFrameShape(QFrame::StyledPanel);
  panel_frame->setAutoFillBackground(true);
  panel_ = panel_frame;

  auto centering_row = new QHBoxLayout;
  centering_row->addStretch();
  centering_row->addWidget(panel_);
  centering_row->addStretch();

  auto outer = new QVBoxLayout(this);
  outer->addStretch();
  outer->addLayout(centering_row);
  outer->addStretch();

  setGeometry(host_->rect());
  hide();
}

QWidget *InlinePopup::ContentPanel() {
  return panel_;
}

int InlinePopup::Exec() {
  setGeometry(host_->rect());
  show();
  raise();

  QEventLoop loop;
  loop_ = &loop;
  loop.exec();
  loop_ = nullptr;

  return result_;
}

void InlinePopup::Accept() {
  Finish(Accepted);
}

void InlinePopup::Reject() {
  Finish(Rejected);
}

void InlinePopup::Finish(int result) {
  result_ = result;
  hide();
  if (loop_ != nullptr) {
    loop_->quit();
  }
}

void InlinePopup::mousePressEvent(QMouseEvent *event) {
  // Only reached for taps outside panel_ -- Qt delivers mouse events
  // directly to whichever child widget is under the point.
  Reject();
  event->accept();
}
