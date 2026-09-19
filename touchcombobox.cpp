#include "touchcombobox.h"

#include <QEventLoop>
#include <QListView>
#include <QModelIndex>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QWidget>

namespace {

// A plain child widget of the top-level window, used as the combo box's
// popup instead of a separate native window. Blocks (via a nested event
// loop) until an item is picked or the user taps outside it, then
// returns the picked row, or -1 if dismissed without a pick.
class InlinePopup : public QWidget {
 public:
  explicit InlinePopup(QWidget *host) : QWidget{host}, host_{host} {
    setGeometry(host_->rect());
  }

  int Exec(QListView *view) {
    auto layout = new QVBoxLayout(this);
    layout->addWidget(view);
    connect(view, &QListView::clicked, this,
        [this](const QModelIndex &index) {
          PickRow(index.row());
        });

    setGeometry(host_->rect());
    show();
    raise();

    QEventLoop loop;
    loop_ = &loop;
    loop.exec();
    loop_ = nullptr;

    return picked_row_;
  }

 protected:
  void mousePressEvent(QMouseEvent *event) override {
    // Only reached for taps outside the list view -- Qt delivers mouse
    // events directly to whichever child widget is under the point.
    PickRow(-1);
    event->accept();
  }

 private:
  void PickRow(int row) {
    picked_row_ = row;
    hide();
    if (loop_ != nullptr) {
      loop_->quit();
    }
  }

  QWidget *host_;
  QEventLoop *loop_ = nullptr;
  int picked_row_ = -1;
};

}  // namespace

void TouchComboBox::showPopup() {
  InlinePopup popup(window());

  auto view = new QListView(&popup);
  view->setModel(model());
  view->setCurrentIndex(model()->index(currentIndex(), 0));

  // Touch-friendly row height, relative to this combo's own font
  // metrics rather than a hard-coded pixel count.
  int row_height = fontMetrics().height() * 3;
  view->setStyleSheet(
      QString("QListView::item { min-height: %1px; }").arg(row_height));

  int picked_row = popup.Exec(view);
  if (picked_row >= 0) {
    setCurrentIndex(picked_row);
  }
}
