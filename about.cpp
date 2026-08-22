#include "about.h"
#include <QFont>
#include <QLabel>
#include <QMainWindow>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>

QWidget* CreateAboutPage(QMainWindow *mainwin) {
  QWidget* page = new QWidget(mainwin);
  QVBoxLayout *mainLayout = new QVBoxLayout(page);

  QLabel *title = new QLabel("ModiMidi", page);
  title->setAlignment(Qt::AlignHCenter);
  QFont font = title->font();
  font.setPointSize(3*font.pointSize());
  font.setBold(true);
  title->setFont(font);

  // Icon from embedded Qt resource (:/icon.png, set up in CMakeLists.txt)
  QLabel *iconLabel = new QLabel(page);
  QPixmap pixmap(":/icon.png");
  if (!pixmap.isNull()) {
    // Scale to a reasonable size relative to the title font height,
    // keeping aspect ratio, never hard-coding pixel dimensions.
    int iconSize = title->fontMetrics().height() * 4;
    iconLabel->setPixmap(
        pixmap.scaled(iconSize, iconSize,
                      Qt::KeepAspectRatio,
                      Qt::SmoothTransformation));
  }
  iconLabel->setAlignment(Qt::AlignHCenter);

  QLabel *summary = new QLabel("Modifiable Midi file player.", page);
  summary->setAlignment(Qt::AlignHCenter);
  font = summary->font();
  font.setPointSize(2*font.pointSize());
  summary->setFont(font);

  // Clickable URL via rich text; QLabel opens links when
  // setOpenExternalLinks(true) is set.
  QLabel *see = new QLabel(
      R"(See: <a href="https://github.com/yotam-medini/modimidi">)"
      R"(https://github.com/yotam-medini/modimidi</a>)", page);
  see->setAlignment(Qt::AlignHCenter);
  see->setTextFormat(Qt::RichText);
  see->setOpenExternalLinks(true);

  mainLayout->addStretch();
  mainLayout->addWidget(title, 2);
  mainLayout->addWidget(iconLabel, 2);
  mainLayout->addWidget(summary, 1);
  mainLayout->addWidget(see, 1);
  mainLayout->addStretch();

  return page;
}
