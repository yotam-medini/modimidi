#include "mixer.h"
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
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
  midi::Midi::channels_range_t channels_range_;
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

  QTableWidget *table = tables_[i] = new QTableWidget(frame);
  table->setColumnCount(2);
  table->setHorizontalHeaderLabels({entity_name, "Volume Control"});
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  layout->addLayout(tr_layout);
  layout->addWidget(table);

  return frame;
}

void Mixer::Impl::ResetByMidi() {
  tables_[E_Tracks]->clear();
  tables_[E_Channels]->clear();
  const midi::Midi *parsed_midi = gplay_.GetMidi();
  if (parsed_midi) {
    unsigned n_tracks = parsed_midi->GetNumTracks();
    qDebug() << qFormat("{} n_tracks={}", __func__, n_tracks);
    auto table = tables_[E_Tracks];
    table->setHorizontalHeaderLabels({"Track", "Volume Control"});
    const auto &tracks_ = parsed_midi->GetTracks();
    table->setColumnCount(2);
    table->setRowCount(tracks_.size());
    for (unsigned i = 0; i < n_tracks; ++i) {
      const auto &track = tracks_[i];
      auto cell = new QWidget(table);
      auto layout = new QVBoxLayout(cell);
      auto name_label = new QLabel(track.GetName().c_str(), cell);
      auto details = qFormat("{} Volume: {}",
        track.GetNotesRange(), track.GetVolumeRange());
      auto details_label = new QLabel(details, cell);
      layout->addWidget(name_label);
      layout->addWidget(details_label);
      table->setCellWidget(i, 0, cell);
    }
    table->verticalHeader()->setVisible(false);
    table->resizeRowsToContents();
    table->resizeColumnsToContents();

    channels_range_ = parsed_midi->GetChannelsRange();
    table = tables_[E_Channels];
    table->setHorizontalHeaderLabels({"Channel", "Volume Control"});
    table->setColumnCount(2);
    table->setRowCount(channels_range_.size());
    int row;
    for (const auto &item: channels_range_) {
      const auto &range = item.second;
      const auto s = qFormat("{:2d} [{}, {}]", item.first, range[0], range[1]);
      table->setCellWidget(row++, 0, new QLabel(s, table));
    }
    table->verticalHeader()->setVisible(false);
    table->resizeRowsToContents();
    table->resizeColumnsToContents();
  }
}

Mixer::Mixer(QWidget *page, GPlay &gplay) :
  impl_{std::make_unique<Impl>(page, gplay)} {
}

void Mixer::ResetByMidi() {
  impl_->ResetByMidi();
}
