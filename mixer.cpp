#include "mixer.h"
#include <algorithm>
#include <array>
#include <QComboBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

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
  void SetTracksTable();
  void SetChannelsTable();
  QWidget *CreateControlWidget(QWidget *parent, unsigned e_mixable, int i);
  GPlay &gplay_;
  QPushButton *default_buttons_[E_N]{nullptr, nullptr};
  QPushButton *silence_buttons_[E_N]{nullptr, nullptr};
  QTableWidget *tables_[E_N]{nullptr, nullptr};
  midi::Midi::channels_range_t channels_range_;
};

void Mixer::Impl::CreateUI(QWidget *page) {
  auto main_layout = new QVBoxLayout(page);
  auto splitter = new QSplitter(page);
  splitter->setOrientation(Qt::Vertical);
  splitter->addWidget(CreateFrame(page, E_Tracks));
  splitter->addWidget(CreateFrame(page, E_Channels));
  main_layout->addWidget(splitter);
  page->setLayout(main_layout);
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
  default_buttons_[i] = new QPushButton("Default", frame);
  silence_buttons_[i] = new QPushButton("Silence", frame);
  QHBoxLayout *tr_layout = new QHBoxLayout(frame);
  tr_layout->addWidget(title, 2);
  tr_layout->addWidget(default_buttons_[i], 1);
  tr_layout->addWidget(silence_buttons_[i], 1);

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
  SetTracksTable();
  SetChannelsTable();
}

void Mixer::Impl::SetTracksTable() {
  tables_[E_Tracks]->clear();
  const midi::Midi *parsed_midi = gplay_.GetMidi();
  if (parsed_midi) {
    const auto &tracks = parsed_midi->GetTracks();
    const auto n_tracks = tracks.size();
    qDebug() << qFormat("{} n_tracks={}", __func__, n_tracks);
    auto table = tables_[E_Tracks];
    table->setHorizontalHeaderLabels({"Track", "Volume Control"});
    const auto &tracks_ = parsed_midi->GetTracks();
    table->setColumnCount(2);
    const unsigned n_rows = std::count_if(
      tracks.begin(), tracks.end(),
      [](const midi::Track &t) { return t.HasNotes(); });
    table->setRowCount(n_rows);
    unsigned row = 0;
    for (size_t i = 0; i < n_tracks; ++i) {
      const auto &track = tracks_[i];
      if (track.HasNotes()) {
        auto cell = new QWidget(table);
        auto layout = new QVBoxLayout(cell);
        auto name_label = new QLabel(track.GetName().c_str(), cell);
        auto details = qFormat("{} Volume: {}",
          midi::MidiNoteRangeToString(track.GetKeyRange()),
          midi::RangeToString(track.GetVelocityRange()));
        auto details_label = new QLabel(details, cell);
        layout->addWidget(name_label);
        layout->addWidget(details_label);
        table->setCellWidget(row, 0, cell);
        table->setCellWidget(row, 1, CreateControlWidget(table, E_Tracks, i));
        ++row;
      }
    }
    table->verticalHeader()->setVisible(false);
    table->resizeRowsToContents();
    table->resizeColumnsToContents();
  }
}

void Mixer::Impl::SetChannelsTable() {
  const midi::Midi *parsed_midi = gplay_.GetMidi();
  if (parsed_midi) {
    auto table = tables_[E_Channels];
    table->clear();
    channels_range_ = parsed_midi->GetChannels();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Channel", "Volume Control"});
    table->setRowCount(channels_range_.size());
    int row = 0;
    for (const auto &[ci, range]: channels_range_) {
      auto const details = std::format("{} Volume: {}",
        midi::MidiNoteRangeToString(range.notes_range_),
        midi::RangeToString(range.velocity_range_));
      const auto s = qFormat("{:2d} {}", ci, details);
      table->setCellWidget(row, 0, new QLabel(s, table));
      table->setCellWidget(row, 1, CreateControlWidget(table, E_Channels, ci));
      ++row;
    }
    table->verticalHeader()->setVisible(false);
    table->resizeRowsToContents();
    table->resizeColumnsToContents();
  }
}

QWidget *Mixer::Impl::CreateControlWidget(
  QWidget *parent,
  unsigned e_mixable,
  int i) {
  qDebug() << qFormat("{} e_mixable={}, i={}\n", __func__, e_mixable, i);
  auto w = new QWidget(parent);
  auto layout = new QHBoxLayout(w);

  QComboBox* combo = new QComboBox(w);
  combo->addItem("Default");
  combo->addItem("Silence");
  combo->addItem("Custom");

  connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
    [this, e_mixable, i, combo](int index) {
      qDebug() << "Selected" << combo->currentText() 
               << "for" << e_mixable << i;
  });

  layout->addWidget(combo);
  w->setLayout(layout);
  return w;
}

Mixer::Mixer(QWidget *page, GPlay &gplay) :
  impl_{std::make_unique<Impl>(page, gplay)} {
}

void Mixer::ResetByMidi() {
  impl_->ResetByMidi();
}
