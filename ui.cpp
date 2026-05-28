#include "ui.h"
#include "mainwin.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <utility>
#include <vector>

#include <QAction>
#include <QApplication>
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
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "debug.h"
#include "fmtqstr.h"
#include "gplay.h"
#include "qutil.h"

MainWindow::MainWindow(GPlay &gplay) : 
     QMainWindow(nullptr),
     gplay_{gplay} {
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

  actions_[Pause] = new QAction(this);
  actions_[Stop] = new QAction(this);
  actions_[Play] = new QAction(this);
  actions_[Forward] = new QAction(this);
  actions_[Backward] = new QAction(this);
  for (auto action: actions_) {
    action->setEnabled(false);
  }

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
    a->setEnabled(false);
  };
  init_button(Pause, QStyle::SP_MediaPause);
  init_button(Stop, QStyle::SP_MediaStop);
  init_button(Play, QStyle::SP_MediaPlay);
  init_button(Forward, QStyle::SP_MediaSeekForward);
  init_button(Backward, QStyle::SP_MediaSeekBackward);

  // 4. Assemble the Layouts
  // Add "Springs" (stretch) to center the buttons horizontally
  buttonLayout->addStretch(); 
  for (auto b : buttons_) {
    buttonLayout->addWidget(b);
  }
  buttonLayout->addStretch();

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

  // Add "Springs" to center the horizontal row vertically
  mainLayout->addStretch();    // Pushes everything down
  mainLayout->addLayout(buttonLayout);
  mainLayout->addLayout(progressLayout);    // Pushes everything up
  mainLayout->addStretch();    // Pushes everything up

  connect(actions_[Play], &QAction::triggered, this, [this, progress_label]() {
    std::cerr << std::format("{}:{}\n", __FILE__, __LINE__);
    actions_[Stop]->setEnabled(true);
    gplay_.Play();
  });
  connect(actions_[Stop], &QAction::triggered, this, [this]() {
    std::cerr << std::format("{}:{}\n", __FILE__, __LINE__);
    gplay_.Stop();
  });
  DebugMessage::AddMessage("MainWindow constructed");
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
    reOpenAction->setEnabled(true);
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
    actions_[Play]->setEnabled(err.empty());
    actions_[Pause]->setEnabled(err.empty());
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

class UI::Impl {
 public:
  Impl(int argc, char **argv, GPlay &gplay, bool is_android) :
      argc_{argc},
      argv_{argv},
      app_{argc_, argv_},
      window_{gplay} {

    window_.setWindowTitle("ModiMidi");
    if (is_android) {
      window_.showMaximized();
    } else {
      window_.resize(400, 300);
    }
  }

  int Run() {
    window_.show();
    return app_.exec();
  }
 private:
   int argc_;
   char **argv_;
   QApplication app_;
   MainWindow window_;
};

UI::UI(int argc, char **argv, GPlay &gplay, bool is_android) :
  impl_{std::make_unique<Impl>(argc, argv, gplay, is_android)} {
}

UI::~UI() {
}

int UI::Run() {
  int rc = impl_->Run();
  return rc;
}
