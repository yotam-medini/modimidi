#include "mixer.h"
#include <algorithm>
#include <algorithm>
#include <charconv>
#include <QComboBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "gplay.h"
#include "midi.h"
#include "qutil.h"
#include "buttonedit.h"
#include "rangeslider.h"


class Mixer::Impl {
 public:
  Impl(QWidget *page, GPlay &gplay) :
    gplay_{gplay} {
    CreateUI(page);
  }
  void ResetByMidi();
 private:
  enum { E_Tracks, E_Channels, E_MixableCount };
  enum { E_ComboDefault, E_ComboSilence, E_ComboCustom, E_ComboCount };
  using range_t = midi::Midi::range_t;
  QFrame* CreateFrame(QWidget *page, unsigned i);
  void CreateUI(QWidget *page);
  void SetTracksTable();
  void SetChannelsTable();
  QWidget *CreateControlWidget(QWidget *parent, unsigned e_mixable, int i);
  std::string ParseLowHigh(
    unsigned mixable,
    size_t i,
    RangeSlider *range_slider,
    const std::string &in,
    std::string &out);
  GPlay &gplay_;
  QPushButton *default_buttons_[E_MixableCount]{nullptr, nullptr};
  QPushButton *silence_buttons_[E_MixableCount]{nullptr, nullptr};
  QTableWidget *tables_[E_MixableCount]{nullptr, nullptr};
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
  combo->addItem("Default", static_cast<int>(E_ComboDefault));
  combo->addItem("Silence", static_cast<int>(E_ComboSilence));
  combo->addItem("Custom", static_cast<int>(E_ComboCustom));

  const midi::Midi *parsed_midi = gplay_.GetMidi();
  std::function<range_t()> get_range;
  if (e_mixable == E_Tracks) {
    get_range = [parsed_midi, i]() -> range_t {
      return parsed_midi->GetTracks()[i].GetVolumeRange();
    };
  } else {
    get_range = [parsed_midi, i]() -> range_t {
      const auto &channels_range = parsed_midi->GetChannels();
      auto iter = channels_range.find(i);
      return iter->second.velocity_range_;
    };
  }
  auto get_edit_value = [get_range]() -> std::string {
     const range_t range = get_range();
     return std::format("{},{}", range[0], range[1]);
  };
  QRegularExpression rx("\\d{1,3},\\d{1,3}");
  auto validator = new QRegularExpressionValidator(rx);
  auto range_slider = new RangeSlider(w);
  auto button_edit = new ButtonEditable(
    get_edit_value(), w, "Volume Range", "Set Volume Range\n0⩽low,high<128",
    get_edit_value,
    validator,
    [this, e_mixable, i, range_slider]
      (const std::string &in, std::string &out) -> std::string {
      return ParseLowHigh(e_mixable, i, range_slider, in, out);
    }
  );
  button_edit->setEnabled(false);

  const auto default_range = get_range();
  qDebug() << qFormat("default_range={},{}", default_range[0], default_range[1]);
  range_slider->setEnabled(true);
  range_slider->SetRange(0, 127);
  range_slider->SetValues(default_range[0], default_range[1]);
  range_slider->setEnabled(false);

  auto vlayout = new QVBoxLayout(w);
  vlayout->addWidget(button_edit);
  vlayout->addWidget(range_slider);

  connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
    [this, e_mixable, i, combo, range_slider, button_edit, get_range]
        (int index) {
      qDebug() << "Selected" << combo->currentText() 
        << " currentInex=" << combo->currentIndex()
               << "for" << e_mixable << i;
      auto range = get_range();
      switch (combo->currentIndex()) {
       case E_ComboDefault:
        button_edit->setEnabled(false);
        range_slider->setEnabled(false);
        break;
       case E_ComboSilence:
        button_edit->setEnabled(false);
        range_slider->setEnabled(false);
        range = range_t{0, 0};
        break;
       case E_ComboCustom:
        button_edit->setEnabled(true);
        range_slider->setEnabled(true);
        break;
       default:
        qDebug() << qFormat("{}:{} unexpected index={}",
          __FILE__, __LINE__, combo->currentIndex());
      }
      range_slider->SetValues(range[0], range[1]);
      button_edit->setText(qFormat("{},{}", range[0], range[1]));
    }
  );

  layout->addWidget(combo);
  layout->addLayout(vlayout);
  w->setLayout(layout);
  return w;
}

std::string Mixer::Impl::ParseLowHigh(
    unsigned mixable,
    size_t i,
    RangeSlider *range_slider,
    const std::string &in,
    std::string &out) {
  std::string error;
  auto comma_pos = in.find(',');
  if (comma_pos == std::string::npos) {
    error = "Missing comma";
  } else {
    uint8_t low = 0xff, high = 0xff;
    auto data = in.data();
    auto result = std::from_chars(data, data + comma_pos, low);
    if (result.ec != std::errc()) {
      error = std::format("Failed to parse low: {}", in.substr(0, comma_pos));
    } else {
      result = std::from_chars(data + comma_pos + 1, data + in.size(), high);
      if (result.ec != std::errc()) {
        error = std::format("Failed to parse high: {}",
          in.substr(comma_pos + 1));
      }
    }
    if (error.empty()) {
      if (low > high) {
        error = std::format("low={} must ⩽ high={}", low, high);
      } else if (high >= 128) {
        error = std::format("high={} must be < 128", high);
      } else {
        out = in;
        range_slider->SetValues(low, high);
      }
    }
  }
  return error;
}

Mixer::Mixer(QWidget *page, GPlay &gplay) :
  impl_{std::make_unique<Impl>(page, gplay)} {
}

void Mixer::ResetByMidi() {
  impl_->ResetByMidi();
}
