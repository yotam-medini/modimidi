#include "mixer.h"
#include <QFont>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>


class Mixer::Impl {
 public:
  Impl(QWidget *page, GPlay &gplay) :
    gplay_{gplay} {
    CreateUI(page);
  }
 private:
  void CreateUI(QWidget *page);
  GPlay &gplay_;
};

void Mixer::Impl::CreateUI(QWidget *page) {
  QVBoxLayout *layout = new QVBoxLayout(page);
  QFont font;

  auto tracks_title = new QLabel("Tracks Mixer", page);
  tracks_title->setAlignment(Qt::AlignHCenter);
  font = tracks_title->font();
  font.setPointSize(2*font.pointSize());
  font.setBold(true);
  tracks_title->setFont(font);

  auto channels_title = new QLabel("Channels Mixer", page);
  channels_title->setAlignment(Qt::AlignHCenter);
  font = channels_title->font();
  font.setPointSize(2*font.pointSize());
  font.setBold(true);
  channels_title->setFont(font);

  layout->addWidget(tracks_title);
  layout->addWidget(channels_title);
}

Mixer::Mixer(QWidget *page, GPlay &gplay) :
  impl_{std::make_unique<Impl>(page, gplay)} {
}


