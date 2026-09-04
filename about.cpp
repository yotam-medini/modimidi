#include "about.h"
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>
#include "version.h"
#include "qutil.h"

QWidget* CreateAboutPage(QMainWindow *mainwin) {
  QWidget* page = new QWidget(mainwin);
  QVBoxLayout *main_layout = new QVBoxLayout(page);
  QHBoxLayout *title_layout = new QHBoxLayout(page);

  QLabel *title = new QLabel("ModiMidi", page);
  QFont font = title->font();
  font.setPointSize(3*font.pointSize());
  font.setBold(true);
  title->setFont(font);

  QLabel *label_version = new QLabel(qFormat(" {}", version), page);

  QFontMetrics title_metrics(title->font());
  QFontMetrics version_metrics(label_version->font());
  int offset = title_metrics.ascent() - version_metrics.ascent();

  title_layout->addWidget(title, 0, Qt::AlignTop);
  title_layout->addWidget(label_version, 0, Qt::AlignTop);

  // Adjust version label's vertical position to align base lines
  if (offset > 0) {
    title_layout->setContentsMargins(0, 0, 0, 0);
    label_version->setContentsMargins(0, offset, 0, 0);
  }

  title_layout->setAlignment(Qt::AlignHCenter);

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

  main_layout->addStretch();
  main_layout->addLayout(title_layout, 2);
  main_layout->addWidget(iconLabel, 2);
  main_layout->addWidget(summary, 1);
  main_layout->addWidget(see, 1);
  main_layout->addStretch();

  return page;
}
