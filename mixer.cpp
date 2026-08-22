#include "mixer.h"
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "gplay.h"
#include "midi.h"
#include "qutil.h"


class Mixer::Impl {
 public:
  Impl(QWidget *page, GPlay &gplay) :
    gplay_{gplay} {
    CreateUI(page);
  }
  void ResetByMidi();
 private:
  enum { E_Tracks, E_Channels, E_N };
  QFrame* CreateFrame(QWidget *page, unsigned i);
  void CreateUI(QWidget *page);
  GPlay &gplay_;
  QPushButton *reset_buttons_[E_N]{nullptr, nullptr};
  QTableWidget *tables_[E_N]{nullptr, nullptr};
};

void Mixer::Impl::CreateUI(QWidget *page) {
  QVBoxLayout *main_layout = new QVBoxLayout(page);
  main_layout->addWidget(CreateFrame(page, E_Tracks));
  main_layout->addWidget(CreateFrame(page, E_Channels));
}

QFrame* Mixer::Impl::CreateFrame(QWidget *page, unsigned i) {
  const char *entity_name = (i == E_Tracks ? "Track" : "Channel");
  auto frame = new QFrame(page);
  frame->setFrameShape(QFrame::Box);
  QVBoxLayout *layout = new QVBoxLayout(frame);
  const auto title_name = qFormat("{} Mixer", entity_name);
  auto title = new QLabel(title_name, frame);
  title->setAlignment(Qt::AlignHCenter);
  QFont font = title->font();
  font.setPointSize((3*font.pointSize())/2);
  font.setBold(true);
  title->setFont(font);
  reset_buttons_[i] = new QPushButton("Reset", frame);
  QHBoxLayout *tr_layout = new QHBoxLayout(frame);
  tr_layout->addWidget(title, 2);
  tr_layout->addWidget(reset_buttons_[i], 1);

  QTableWidget *table = new QTableWidget(frame);
  table->setColumnCount(2);
  table->setHorizontalHeaderLabels({entity_name, "Volume Control"});
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  layout->addLayout(tr_layout);
  layout->addWidget(table);
  
  return frame;
}

void Mixer::Impl::ResetByMidi() {
  const midi::Midi *parsed_midi = gplay_.GetMidi();
  if (parsed_midi) {
    auto n_tracks = parsed_midi->GetNumTracks();
    qDebug() << qFormat("{} n_tracks={}", __func__, n_tracks);
  }
}

Mixer::Mixer(QWidget *page, GPlay &gplay) :
  impl_{std::make_unique<Impl>(page, gplay)} {
}

void Mixer::ResetByMidi() {
  impl_->ResetByMidi();
}
