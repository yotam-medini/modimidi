#include "mixer.h"
#include <QFont>
#include <QFrame>
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
  QVBoxLayout *main_layout = new QVBoxLayout(page);
  QFont font;

  auto tracks_frame = new QFrame(page);
  tracks_frame->setFrameShape(QFrame::Box);
  QVBoxLayout *tracks_layout = new QVBoxLayout(tracks_frame);
  auto tracks_title = new QLabel("Tracks Mixer", tracks_frame);
  tracks_title->setAlignment(Qt::AlignHCenter);
  font = tracks_title->font();
  font.setPointSize(2*font.pointSize());
  font.setBold(true);
  tracks_title->setFont(font);
  tracks_layout->addWidget(tracks_title);

  auto channels_frame = new QFrame(page);
  channels_frame->setFrameShape(QFrame::Box);
  QVBoxLayout *channels_layout = new QVBoxLayout(channels_frame);
  auto channels_title = new QLabel("Channels Mixer", channels_frame);
  channels_title->setAlignment(Qt::AlignHCenter);
  font = channels_title->font();
  font.setPointSize(2*font.pointSize());
  font.setBold(true);
  channels_title->setFont(font);
  channels_layout->addWidget(channels_title);

  main_layout->addWidget(tracks_frame);
  main_layout->addWidget(channels_frame);
}

Mixer::Mixer(QWidget *page, GPlay &gplay) :
  impl_{std::make_unique<Impl>(page, gplay)} {
}


