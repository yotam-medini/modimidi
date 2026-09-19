#include "touchcombobox.h"

#include <QListView>
#include <QModelIndex>
#include <QVBoxLayout>

#include "inlinepopup.h"

void TouchComboBox::showPopup() {
  InlinePopup popup(window());
  QWidget *panel = popup.ContentPanel();

  auto layout = new QVBoxLayout(panel);
  auto view = new QListView(panel);
  view->setModel(model());
  view->setCurrentIndex(model()->index(currentIndex(), 0));

  // Touch-friendly row height, relative to this combo's own font
  // metrics rather than a hard-coded pixel count.
  int row_height = fontMetrics().height() * 3;
  view->setStyleSheet(
      QString("QListView::item { min-height: %1px; }").arg(row_height));
  layout->addWidget(view);

  int picked_row = -1;
  connect(view, &QListView::clicked, &popup,
      [&popup, &picked_row](const QModelIndex &index) {
        picked_row = index.row();
        popup.Accept();
      });

  int result = popup.Exec();
  if (result == InlinePopup::Accepted && picked_row >= 0) {
    setCurrentIndex(picked_row);
  }
}
