#include "touchcombobox.h"

#ifdef Q_OS_ANDROID
#include <QDialog>
#include <QListWidget>
#include <QScreen>
#include <QVBoxLayout>
#endif

void TouchComboBox::showPopup() {
#if !defined(Q_OS_ANDROID)
  QComboBox::showPopup();
#else
  QDialog dialog(this, Qt::Dialog | Qt::WindowTitleHint);
  if (!accessibleName().isEmpty()) {
    dialog.setWindowTitle(accessibleName());
  }

  QListWidget *list = new QListWidget(&dialog);
  for (int i = 0; i < count(); ++i) {
    new QListWidgetItem(itemText(i), list);
  }
  if (currentIndex() >= 0) {
    list->setCurrentRow(currentIndex());
  }

  // Touch-friendly row height, relative to the list's own font metrics
  // rather than a hard-coded pixel count.
  int row_height = list->fontMetrics().height() * 2;
  list->setStyleSheet(
      QString("QListWidget::item { min-height: %1px; }").arg(row_height));

  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  layout->addWidget(list);

  connect(list, &QListWidget::itemActivated, &dialog, &QDialog::accept);
  connect(list, &QListWidget::itemClicked, &dialog, &QDialog::accept);

  // Size relative to the screen, not the (often narrow) combo box width,
  // so items are easy to hit.
  if (QScreen *screen = this->screen()) {
    QSize screen_size = screen->availableSize();
    dialog.resize(screen_size.width() * 3 / 4, screen_size.height() / 2);
  }

  if (dialog.exec() == QDialog::Accepted) {
    QListWidgetItem *chosen = list->currentItem();
    if (chosen) {
      setCurrentIndex(list->row(chosen));
    }
  }
#endif
}
