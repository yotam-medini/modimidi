#include "about.h"
#include <QFont>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>

QWidget* CreateAboutPage(QMainWindow *mainwin) {
  QWidget* page = new QWidget(mainwin);
  QVBoxLayout *mainLayout = new QVBoxLayout(page);

  // QHBoxLayout* fileRow = new QHBoxLayout;
  QLabel *title = new QLabel("ModiMidi", page);
  // title->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  title->setAlignment(Qt::AlignHCenter);
  QFont font = title->font();
  font.setPointSize(3*font.pointSize());
  font.setBold(true);
  title->setFont(font);

  QLabel *summary = new QLabel("Modifiable Midi file player.");
  summary->setAlignment(Qt::AlignHCenter);
  font = summary->font();
  font.setPointSize(2*font.pointSize());

  QLabel *see = new QLabel("See: https://github.com/yotam-medini/modimidi");
  see->setAlignment(Qt::AlignHCenter);

  mainLayout->addStretch();
  mainLayout->addWidget(title, 2);
  mainLayout->addWidget(summary, 1);
  mainLayout->addWidget(see, 1);
  mainLayout->addStretch();

  return page;
}
