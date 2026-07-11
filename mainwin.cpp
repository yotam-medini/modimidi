#include "mainwin.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QStyle>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QValidator>
#include <QVBoxLayout>

#include "debug.h"
#include "gplay.h"
#include "qutil.h"
#include "rangeslider.h"
#include "util.h"

namespace {

// Formats ms as "MM:SS.mmm" (2-digit minutes/seconds, 3-digit
// millis), for pre-filling the range-edit text-input dialog. This
// happens to be one valid input for ParseMmSsMmm() below (a fully
// zero-padded one); see ParseMmSsMmm() for the accepted input forms.
QString FormatMmSsDotMmm(uint32_t ms) {
  const uint32_t total_seconds = ms / 1000;
  const uint32_t millis = ms % 1000;
  const uint32_t minutes = total_seconds / 60;
  const uint32_t seconds = total_seconds % 60;
  return QString::fromStdString(std::format(
    "{:02d}:{:02d}.{:03d}", minutes, seconds, millis));
}

// Parses a flexible "MM:SS.mmm" time entry:
//  - MM: 1+ digits (single-digit minutes allowed, e.g. "3:07").
//  - SS: 1-2 digits, must satisfy 0<=SS<60.
//  - The fractional-second part is entirely optional and, when
//    present, is a decimal fraction of a second: "." followed by
//    1-3 digits, e.g. ".5" (500ms), ".25" (250ms), ".250" (250ms).
//    Dropping it (no "." at all, e.g. "3:07") means 0 ms.
// Returns false, leaving *ms_out untouched, on any syntax or range
// violation.
bool ParseMmSsMmm(const QString &text, uint32_t *ms_out) {
  static const std::regex kPattern(
    R"(^\s*(\d+):(\d{1,2})(?:\.(\d{1,3}))?\s*$)");
  const std::string s = text.toStdString();
  std::smatch match;
  if (!std::regex_match(s, match, kPattern)) {
    return false;
  }
  uint32_t mm = 0, ss = 0, mmm = 0;
  try {
    mm = static_cast<uint32_t>(std::stoul(match[1].str()));
    ss = static_cast<uint32_t>(std::stoul(match[2].str()));
    if (ss >= 60) {
      return false;
    }
    // Right-pad the fractional digits to 3 so ".5" == ".500" ==
    // 500ms, ".25" == ".250" == 250ms, missing fraction == 0ms.
    std::string frac = match[3].str();
    while (frac.size() < 3) {
      frac += '0';
    }
    mmm = static_cast<uint32_t>(std::stoul(frac));
  } catch (const std::exception &) {
    return false; // out-of-range digit string, etc.
  }
  if (mmm >= 1000) {
    return false;
  }
  *ms_out = (mm * 60 + ss) * 1000 + mmm;
  return true;
}

// Loose live-typing filter for the time-input QLineEdit: accepts
// only digits, ':' and '.' while the user types. The actual
// MM:SS.mmm syntax + range check happens on OK, via ParseMmSsMmm()
// above. A small std::regex-backed QValidator, in place of Qt's own
// QRegularExpressionValidator, since std::regex covers this need.
class TimeCharsValidator : public QValidator {
 public:
  explicit TimeCharsValidator(QObject *parent) : QValidator(parent) {}

  State validate(QString &input, int &) const override {
    static const std::regex kAllowed(R"(^[0-9:.]*$)");
    return std::regex_match(input.toStdString(), kAllowed) ?
      Acceptable : Invalid;
  }
};

}  // namespace

MainWindow::MainWindow(GPlay &gplay) :
     QMainWindow(nullptr),
     gplay_{gplay} {
  menuBar()->setVisible(false);

  for (auto &action: actions_) {
    action = new QAction(this);
    action->setEnabled(false);
  }

  tabs_ = new QTabWidget(this);
  tabs_->setTabPosition(QTabWidget::South); // bottom tabs – Android friendly
  tabs_->addTab(BuildPlayerPage(), tr("Player"));
  tabs_->addTab(BuildModifyPage(), tr("Modify"));
  tabs_->addTab(BuildInfoPage(),   tr("Info"));
  tabs_->addTab(BuildAboutPage(),  tr("About"));
  setCentralWidget(tabs_);


  DebugMessage::AddMessage("MainWindow constructed");
}

QWidget* MainWindow::BuildPlayerPage() {
  QWidget* page = new QWidget;
  // 2. Main Vertical Layout (This fills the whole central area)
  QVBoxLayout *mainLayout = new QVBoxLayout(page);

  QHBoxLayout* fileRow = new QHBoxLayout;

  fileButton_ = new QPushButton(tr("(no file)"), page);
  fileButton_->setFlat(true);
  fileButton_->setStyleSheet(
    "text-align: left; font-style: italic; color: gray;");
  fileButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  fileButton_->setEnabled(false); // nothing to show until a file is opened
  connect(
    fileButton_, &QPushButton::clicked,
    this, &MainWindow::showFilePathDialog);

  QToolButton* openBtn = new QToolButton;
  openBtn->setText(tr("Open…"));
  openBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
  connect(openBtn, &QToolButton::clicked, this, &MainWindow::openFile);

  // Task 2: Quit button next to "Open…", with a confirmation dialog.
  QToolButton* quitBtn = new QToolButton;
  quitBtn->setText(tr("Quit"));
  quitBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
  connect(quitBtn, &QToolButton::clicked, this, &MainWindow::confirmQuit);

  fileRow->addWidget(fileButton_, 1);
  fileRow->addWidget(openBtn);
  fileRow->addWidget(quitBtn);
  mainLayout->addLayout(fileRow);


  QHBoxLayout *buttonLayout = new QHBoxLayout();

  auto init_button = [this, page](
      size_t i, QStyle::StandardPixmap icon) -> void {
    auto a = actions_[i];
    auto b = buttons_[i] = new QToolButton(page);
    a->setIcon(style()->standardIcon(icon));
    b->setToolButtonStyle(Qt::ToolButtonIconOnly);
    b->setDefaultAction(a);
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    SetButtonOpColor(i);
    a->setEnabled(false);
  };
  // or SP_MediaSeekForward
  init_button(Op::Pause, QStyle::SP_MediaPause); 

  init_button(Op::Stop, QStyle::SP_MediaStop);
  init_button(Op::Play, QStyle::SP_MediaPlay);
  init_button(Op::Forward, QStyle::SP_MediaSkipForward);
  init_button(Op::Backward, QStyle::SP_MediaSkipBackward);

  // 4. Assemble the Layouts
  // Add "Springs" (stretch) to center the buttons horizontally
  buttonLayout->addStretch(1);
  for (auto b : buttons_) {
    buttonLayout->addWidget(b, 1);
  }
  buttonLayout->addStretch(1);

  QHBoxLayout *progressLayout = new QHBoxLayout();
  QLabel *progress_label = new QLabel("Progress", this);
  progressLayout->addStretch();
  progressLayout->addWidget(progress_label);
  progressLayout->addStretch();

  // Slider + labels are grouped in one widget so they can be shown
  // or hidden together: there is nothing meaningful to select until
  // a proper midi file is loaded, so the group starts hidden.
  rangeGroup_ = new QWidget(page);
  QVBoxLayout *rangeGroupLayout = new QVBoxLayout(rangeGroup_);
  rangeGroupLayout->setContentsMargins(0, 0, 0, 0);

  rangeSlider_ = new RangeSlider(rangeGroup_);
  rangeSlider_->setEnabled(true);
  constexpr uint32_t kPlaceholderMaxMs = 5 * 60 * 1000; // 5:00.000
  rangeSlider_->SetRange(0, kPlaceholderMaxMs);
  rangeSlider_->SetValues(0, kPlaceholderMaxMs);

  QHBoxLayout *rangeLabelsLayout = new QHBoxLayout();
  // Label-buttons (normal raised push-button look, to hint they're
  // clickable): clicking either pops up a "MM:SS.mmm" text-input
  // dialog to set that end of the range to an exact value.
  rangeStartLabel_ = new QPushButton(rangeGroup_);
  rangeStartLabel_->setToolTip(tr("Click to enter an exact time"));
  rangeEndLabel_ = new QPushButton(rangeGroup_);
  rangeEndLabel_->setToolTip(tr("Click to enter an exact time"));
  UpdateRangeStartLabel(rangeSlider_->LowValue());
  UpdateRangeEndLabel(rangeSlider_->HighValue());
  connect(
    rangeStartLabel_, &QPushButton::clicked,
    this, &MainWindow::editRangeStart);
  connect(
    rangeEndLabel_, &QPushButton::clicked,
    this, &MainWindow::editRangeEnd);
  rangeLabelsLayout->addWidget(rangeStartLabel_);
  rangeLabelsLayout->addStretch();
  rangeLabelsLayout->addWidget(rangeEndLabel_);

  rangeGroupLayout->addWidget(rangeSlider_);
  rangeGroupLayout->addLayout(rangeLabelsLayout);
  rangeGroup_->setVisible(false); // hidden until a midi file loads

  connect(
      rangeSlider_, &RangeSlider::lowValueChanged, this,
      [this](uint32_t ms) { UpdateRangeStartLabel(ms); });
  connect(
      rangeSlider_, &RangeSlider::highValueChanged, this,
      [this](uint32_t ms) { UpdateRangeEndLabel(ms); });
  // Note: RangeSlider::rangeEdited (emitted once a handle is released,
  // or once a text-input edit is committed via CommitEdit()) is
  // intentionally left unconnected here — hook it up to whatever
  // playback/sub-segment logic is appropriate.

  gplay_.SetProgressCallback([progress_label](
      uint32_t done_ms,
      uint32_t final_ms,
      const std::string& mmss_done,
      const std::string& mmss_final) {
    auto const s = std::format("Progress: {} / {}", mmss_done, mmss_final);
    progress_label->setText(QString::fromStdString(s));
  });
  gplay_.SetStateCallback([this](State state) { OnStateChange(state); });

  // Add "Springs" to center the horizontal row vertically
  mainLayout->addStretch();    // Pushes everything down
  mainLayout->addLayout(buttonLayout);
  mainLayout->addLayout(progressLayout);
  mainLayout->addWidget(rangeGroup_);
  mainLayout->addStretch();    // Pushes everything up

  ConnectButtonsActions();

  return page;
}

QWidget* MainWindow::BuildModifyPage() {
  QWidget* page = new QWidget;

  return page;
}

QWidget* MainWindow::BuildInfoPage() {
  QWidget* page = new QWidget;

  return page;
}

QWidget* MainWindow::BuildAboutPage() {
  QWidget* page = new QWidget;

  return page;
}

void MainWindow::openFile() {
  qDebug() << std::format("{}:{} {}", __FILE__, __LINE__, __func__);
  QString filter = "MIDI Files (*.mid *.MID *.midi *.MIDI);;All Files (*)";

  QString fileName = QFileDialog::getOpenFileName(this,
      tr("Open MIDI File"), "", filter);

  qDebug() << std::format("{}:{} fileName={}",
    __FILE__, __LINE__, fileName.toStdString());
  if (!fileName.isEmpty()) {
    lastOpenedPath = fileName;
    // reOpenAction->setEnabled(true);
    // Add your file processing logic here
    std::vector<uint8_t> data;
    std::string err = read_binary_file(fileName, data);
    if (err.empty()) {
      auto const msg = std::format("{}:{} data.size=={}",
        __FILE__, __LINE__, data.size());
      qDebug() << msg;
      DebugMessage::AddMessage(msg);
      err = gplay_.OpenMidi(data);
    }
    if (!err.empty()) {
      qDebug() << std::format("err={}", err);
      QMessageBox::warning(this, "ModiMidi Warning", 
        qFormat("OpenMidi({}) failed:\n{}", fileName.toStdString(), err));
    }
    actions_[Op::Play]->setEnabled(err.empty());
    if (err.empty()) {
      fileButton_->setText(QFileInfo(fileName).fileName());
      fileButton_->setEnabled(true);
      // Task 2: only reveal the range slider once a proper midi
      // file has actually been loaded; reset it to the (still
      // placeholder, pending a real duration from GPlay) full range.
      constexpr uint32_t kPlaceholderMaxMs = 5 * 60 * 1000;
      rangeSlider_->SetRange(0, kPlaceholderMaxMs);
      rangeSlider_->SetValues(0, kPlaceholderMaxMs);
      UpdateRangeStartLabel(rangeSlider_->LowValue());
      UpdateRangeEndLabel(rangeSlider_->HighValue());
      rangeGroup_->setVisible(true);
    }
  }
}

void MainWindow::reOpenFile() {
  if (!lastOpenedPath.isEmpty()) {
      // Logic to reload the file at lastOpenedPath
  }
}

void MainWindow::showFilePathDialog() {
  // Task 1: clicking the file-basename button shows the full path.
  const QString path = lastOpenedPath.isEmpty() ?
    tr("(no file opened)") : lastOpenedPath;
  QMessageBox::information(this, tr("File Path"), path);
}

void MainWindow::UpdateRangeStartLabel(uint32_t ms) {
  rangeStartLabel_->setText(QString::fromStdString(
    std::format("Start: {}", milliseconds_to_string(ms))));
}

void MainWindow::UpdateRangeEndLabel(uint32_t ms) {
  rangeEndLabel_->setText(QString::fromStdString(
    std::format("End: {}", milliseconds_to_string(ms))));
}

void MainWindow::editRangeStart() {
  EditRangeValue(/*is_start=*/true);
}

void MainWindow::editRangeEnd() {
  EditRangeValue(/*is_start=*/false);
}

void MainWindow::EditRangeValue(bool is_start) {
  const uint32_t current = is_start ?
    rangeSlider_->LowValue() : rangeSlider_->HighValue();

  QDialog dialog(this);
  dialog.setWindowTitle(is_start ? tr("Edit Start") : tr("Edit End"));

  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  QLabel *hint = new QLabel(
    tr("Enter time as MM:SS.mmm (e.g. 3:07, 3:07.5, 3:07.250)"),
    &dialog);
  layout->addWidget(hint);

  QLineEdit *edit = new QLineEdit(FormatMmSsDotMmm(current), &dialog);
  // Loose live-typing filter (digits, ':' and '.' only); the actual
  // MM:SS.mmm syntax + range check happens on OK, in ParseMmSsMmm().
  edit->setValidator(new TimeCharsValidator(edit));
  layout->addWidget(edit);

  QLabel *error_label = new QLabel(&dialog);
  error_label->setStyleSheet("color: red;");
  layout->addWidget(error_label);

  QDialogButtonBox *buttons = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  edit->setFocus();
  edit->selectAll();

  uint32_t ms = 0;
  while (dialog.exec() == QDialog::Accepted) {
    if (ParseMmSsMmm(edit->text(), &ms)) {
      if (is_start) {
        rangeSlider_->SetLowValue(ms);
      } else {
        rangeSlider_->SetHighValue(ms);
      }
      // A text-input edit is a committed change, same as a mouse
      // release after a drag.
      rangeSlider_->CommitEdit();
      return;
    }
    error_label->setText(tr(
      "Invalid format. Use MM:SS.mmm (fraction optional), "
      "0<=SS<60."));
  }
}

void MainWindow::confirmQuit() {
  // Task 2: Quit button, with a "Really Quit?" confirmation dialog.
  const auto answer = QMessageBox::question(
    this, tr("Quit"), tr("Really Quit?"),
    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (answer == QMessageBox::Yes) {
    qApp->quit();
  }
}

void MainWindow::showDebugDialog() {
  QDialog *dialog = new QDialog(this);
  dialog->setWindowTitle("Recent Debug Messages");

  const QSize main_size = this->size();
  dialog->resize(main_size.width() * 9 / 10, main_size.height() * 6 / 10);

  QVBoxLayout *layout = new QVBoxLayout(dialog);
  QListWidget *listWidget = new QListWidget(dialog);

  // Row height relative to the list's own font metrics, rather than a
  // hard-coded pixel constant.
  const int row_height = listWidget->fontMetrics().height() * 2;
  listWidget->setStyleSheet(
    QString("QListWidget::item { height: %1px; }").arg(row_height));

  // Add logs from our Logger
  QStringList debug_messages;
  for (auto const &message: DebugMessage::GetMessages()) {
     debug_messages << QString::fromStdString(message);
  }
  listWidget->addItems(debug_messages);

  QPushButton *closeButton = new QPushButton("Close", dialog);

  layout->addWidget(listWidget);
  layout->addStretch(1); // flexible space instead of a fixed-size spacer
  layout->addWidget(closeButton);

  QObject::connect(
    closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

  dialog->exec(); // Modal execution
}

void MainWindow::OnStateChange(State state) {
  qDebug() << std::format("{}:{} {} state={}",
    __FILE__, __LINE__, __func__, static_cast<int>(state));
  auto const in_pause = (state == State::Pause);
  auto pause_icon =
    in_pause ? QStyle::SP_MediaSeekForward : QStyle::SP_MediaPause;
  auto pause_color = in_pause ? "darkGreen" : "magenta";
  auto color_style = std::format("background-color: {}", pause_color);
  buttons_[Op::Pause]->setIcon(style()->standardIcon(pause_icon));
  buttons_[Op::Pause]->setStyleSheet(color_style.c_str());
  switch (state) {
   case State::None: // only play
    for (size_t i = 0; i < Op::N_ButtonOps; ++i) {
      actions_[i]->setEnabled(i == Op::Play);
    }
    break;
   case State::Play: // all but play
    for (size_t i = 0; i < Op::N_ButtonOps; ++i) {
      actions_[i]->setEnabled(i != Op::Play);
    }
    break;
   case State::Pause: // leave as is (State::Play)
    break;
   default:
    qDebug() << std::format("{}:{} {} unexpected state={}",
      __FILE__, __LINE__, __func__, static_cast<int>(state));
    break;
  }
}

void MainWindow::SetButtonOpColor(size_t button_op_index) {
  static const auto i2name = std::unordered_map<size_t, const char*>({
    {Op::Pause, "magenta"},
    {Op::Stop, "red"},
    {Op::Play, "green"},
    {Op::Forward, "blue"},
    {Op::Backward, "yellow"},
  });
  auto iter = i2name.find(button_op_index);
  if (iter != i2name.end()) {
    auto color_style = std::format("background-color: {}", iter->second);
    buttons_[button_op_index]->setStyleSheet(color_style.c_str());
  }
}

void MainWindow::ConnectButtonsActions() {
  using GPlayMethod = void (GPlay::*)();
  using l_idx_gpm_t = std::initializer_list<std::pair<size_t, GPlayMethod>>;
  for (auto [i, gp_method]: l_idx_gpm_t{
      {Op::Pause, &GPlay::PauseResume},
      {Op::Stop, &GPlay::Stop},
      {Op::Play, &GPlay::Play},
      {Op::Forward, &GPlay::SkipForward},
      {Op::Backward, &GPlay::SkipBackward}}) {
    connect(actions_[i], &QAction::triggered, this, [this, i, gp_method]() {
      std::cerr << std::format("{}:{} i={}\n", __FILE__, __LINE__, i);
      (gplay_.*gp_method)();
    });
    connect(actions_[i], &QAction::changed, this, [this, i]() {
      bool is_clickable = actions_[i]->isEnabled();
      buttons_[i]->setAutoRaise(is_clickable);
    });
  }
}
