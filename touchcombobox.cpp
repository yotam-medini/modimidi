#include "touchcombobox.h"

#ifdef Q_OS_ANDROID
#include <QCursor>
#include <QDialog>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>
#endif

void TouchComboBox::showPopup() {
#ifndef Q_OS_ANDROID
  QComboBox::showPopup();
#else
  qDebug() << "TouchComboBox::showPopup() ENTER this=" << this
           << "currentIndex=" << currentIndex();
  QDialog dialog(this, Qt::Dialog | Qt::WindowTitleHint);
  if (!accessibleName().isEmpty()) {
    dialog.setWindowTitle(accessibleName());
  }

  QVBoxLayout *outer_layout = new QVBoxLayout(&dialog);

  QWidget *content = new QWidget(&dialog);
  QVBoxLayout *content_layout = new QVBoxLayout(content);
  content_layout->setSpacing(4);

  // Touch-friendly row height, relative to this combo's own font metrics
  // rather than a hard-coded pixel count.
  int row_height = fontMetrics().height() * 3;

  for (int i = 0; i < count(); ++i) {
    qDebug() << "TouchComboBox popup: creating button i=" << i
             << "text=" << itemText(i);
    QPushButton *button = new QPushButton(itemText(i), content);
    button->setMinimumHeight(row_height);
    if (i == currentIndex()) {
      QFont f = button->font();
      f.setBold(true);
      button->setFont(f);
    }
    content_layout->addWidget(button);
    qDebug() << "TouchComboBox popup: button i=" << i
             << "global_geometry="
             << QRect(button->mapToGlobal(QPoint(0, 0)), button->size());
    connect(button, &QPushButton::clicked, &dialog, [this, &dialog, i]() {
      qDebug() << "TouchComboBox popup: button clicked i=" << i
                << "this=" << this
                << "cursor_global_pos=" << QCursor::pos();
      setCurrentIndex(i);
      qDebug() << "TouchComboBox popup: after setCurrentIndex,"
                << "currentIndex=" << currentIndex();
      dialog.accept();
    });
  }
  content_layout->addStretch();

  QScrollArea *scroll_area = new QScrollArea(&dialog);
  scroll_area->setWidget(content);
  scroll_area->setWidgetResizable(true);
  scroll_area->setFrameShape(QFrame::NoFrame);
  outer_layout->addWidget(scroll_area);

  QPushButton *cancel_button = new QPushButton(tr("Cancel"), &dialog);
  connect(cancel_button, &QPushButton::clicked, &dialog, &QDialog::reject);
  outer_layout->addWidget(cancel_button);

  // Size to the actual item count, not a fixed screen fraction -- a
  // 3-item list shouldn't fill 3/4 of the screen. Still capped relative
  // to the screen so a long list scrolls instead of overflowing it.
  int content_height =
      count() * (row_height + content_layout->spacing()) +
      cancel_button->sizeHint().height() + 40;
  int dialog_width = qMax(width() * 2, 240);
  int dialog_height = content_height;
  if (QScreen *screen = this->screen()) {
    QSize available = screen->availableSize();
    dialog_height = qMin(content_height, available.height() * 2 / 3);
    dialog_width = qMin(dialog_width, available.width() * 4 / 5);
  }
  dialog.resize(dialog_width, dialog_height);

  dialog.exec();
  qDebug() << "TouchComboBox::showPopup() EXIT this=" << this
           << "currentIndex=" << currentIndex();
#endif
}
