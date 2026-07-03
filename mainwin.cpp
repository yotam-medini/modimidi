#include "mainwin.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <utility>
#include <vector>

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QFileDialog>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSpacerItem>
#include <QString>
#include <QStyle>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "debug.h"
#include "gplay.h"
#include "qutil.h"

MainWindow::MainWindow(GPlay &gplay) :
     QMainWindow(nullptr),
     gplay_{gplay} {
  menuBar()->setVisible(false);
#if 0
  // 1. Create Actions
  qDebug() << std::format("{}:{} {}", __FILE__, __LINE__, __func__);
  openAction = new QAction(tr("&Open"), this);
  reOpenAction = new QAction(tr("&Re-Open"), this);
  reOpenAction->setEnabled(false); // Disable until a file is opened
  showDebugAction = new QAction(tr("Debug Messages"));
  quitAction = new QAction(tr("&Quit"), this);

  QToolBar *topMenuBar = addToolBar(tr("Main Menu"));
  topMenuBar->setMovable(false); // Keep it locked at the top
  topMenuBar->setFloatable(false);

  // 2. Create a ToolButton to act as the "File" menu header
  QToolButton *fileButton = new QToolButton(this);
  fileButton->setText(tr("File"));
  fileButton->setPopupMode(QToolButton::InstantPopup);

  // 3. Create the actual Menu and attach it to the button
  QMenu *fileMenu = new QMenu(fileButton);
  fileMenu->addAction(openAction);
  fileMenu->addAction(reOpenAction);
  fileMenu->addAction(showDebugAction);
  fileMenu->addSeparator();
  fileMenu->addAction(quitAction);

  fileButton->setMenu(fileMenu);

  // 4. Add the button to your toolbar
  topMenuBar->addWidget(fileButton);

  // 3. Create the actual Menu and attach it to the button
  QToolButton *optionsButton = new QToolButton(this);
  optionsButton->setText(tr("Options"));
  optionsButton->setPopupMode(QToolButton::InstantPopup);
  QMenu *optionsMenu = new QMenu(optionsButton);
  topMenuBar->addWidget(optionsButton);

  // 4. Connect Signals
  connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
  connect(reOpenAction, &QAction::triggered, this, &MainWindow::reOpenFile);
  connect(
    showDebugAction, &QAction::triggered, this, &MainWindow::showDebugDialog);
  connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
#endif

  for (auto &action: actions_) {
    action = new QAction(this);
    action->setEnabled(false);
  }

#if 0
  // 1. Setup the Central Widget
  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  // 2. Main Vertical Layout (This fills the whole central area)
  QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

  // 3. Horizontal Layout for the buttons (The "Button Bar")
  QHBoxLayout *buttonLayout = new QHBoxLayout();

  auto init_button = [this, centralWidget](
      size_t i, QStyle::StandardPixmap icon) -> void {
    auto a = actions_[i];
    auto b = buttons_[i] = new QToolButton(centralWidget);
    a->setIcon(style()->standardIcon(icon));
    b->setToolButtonStyle(Qt::ToolButtonIconOnly);
    b->setDefaultAction(a);
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    SetButtonOpColor(i);
    a->setEnabled(false);
  };
  init_button(Op::Pause, QStyle::SP_MediaPause); // or SP_MediaSeekForward
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
  mainLayout->addLayout(progressLayout);    // Pushes everything up
  mainLayout->addStretch();    // Pushes everything up

  ConnectButtonsActions();
#endif

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
  fileLabel_ = new QLabel(tr("(no file)"));
  fileLabel_->setWordWrap(false);
  fileLabel_->setStyleSheet("font-style: italic; color: gray;");

  QToolButton* openBtn = new QToolButton;
  openBtn->setText(tr("Open…"));
  openBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
  connect(openBtn, &QToolButton::clicked, this, &MainWindow::openFile);

  fileRow->addWidget(fileLabel_, 1);
  fileRow->addWidget(openBtn);
  mainLayout->addLayout(fileRow);


  // 3. Horizontal Layout for the buttons (The "Button Bar")
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
  init_button(Op::Pause, QStyle::SP_MediaPause); // or SP_MediaSeekForward
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
  mainLayout->addLayout(progressLayout);    // Pushes everything up
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
      fileLabel_->setText(QFileInfo(fileName).fileName());
    }
  }
}

void MainWindow::reOpenFile() {
  if (!lastOpenedPath.isEmpty()) {
      // Logic to reload the file at lastOpenedPath
  }
}

void MainWindow::showDebugDialog() {
  QDialog *dialog = new QDialog(this);
  dialog->setWindowTitle("Recent Debug Messages");

  QVBoxLayout *layout = new QVBoxLayout(dialog);
  QListWidget *listWidget = new QListWidget(dialog);

  listWidget->setStyleSheet("QListWidget::item { height: 40px; }");

  // Add logs from our Logger
  QStringList debug_messages;
  for (auto const &message: DebugMessage::GetMessages()) {
     debug_messages << QString::fromStdString(message);
  }
  listWidget->addItems(debug_messages);

  QPushButton *closeButton = new QPushButton("Close", dialog);

  layout->addWidget(listWidget);
  layout->addSpacerItem(
    new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
  layout->addWidget(closeButton);

  QObject::connect(
    closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

  // Mobile Optimization: Resize dialog to 90% of screen width
  QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
  int width = screenGeometry.width() * 0.9;
  int height = screenGeometry.height() * 0.6;
  dialog->resize(width, height);

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
