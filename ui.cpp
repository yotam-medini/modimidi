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
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

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
  connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

  playAction = new QAction("Play", this);
  pauseAction = new QAction("Pause", this);
  playAction->setEnabled(false);
  pauseAction->setEnabled(false);

  // 1. Setup the Central Widget
  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  // 2. Main Vertical Layout (This fills the whole central area)
  QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

  // 3. Horizontal Layout for the buttons (The "Button Bar")
  QHBoxLayout *buttonLayout = new QHBoxLayout();

  QPushButton *playButton = new QPushButton();
  playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  playButton->addAction(playAction);
  connect(playButton, &QPushButton::clicked, playAction, &QAction::trigger);

  QPushButton *pauseButton = new QPushButton();
  pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));

  // 4. Assemble the Layouts
  // Add "Springs" (stretch) to center the buttons horizontally
  buttonLayout->addStretch(); 
  buttonLayout->addWidget(playButton);
  buttonLayout->addWidget(pauseButton);
  buttonLayout->addStretch();

  // Add "Springs" to center the horizontal row vertically
  mainLayout->addStretch();    // Pushes everything down
  mainLayout->addLayout(buttonLayout);
  mainLayout->addStretch();    // Pushes everything up

  connect(playAction, &QAction::triggered, this, [this]() {
    std::cerr << std::format("{}:{}\n", __FILE__, __LINE__);
    gplay_.Play();
  });
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
      qDebug() << std::format("{}:{} data.size=={}",
        __FILE__, __LINE__, data.size());
      err = gplay_.OpenMidi(data);
    }
    if (!err.empty()) {
      qDebug() << std::format("err={}", err);
      lastOpenedPath = fileName;
      reOpenAction->setEnabled(true);
      // Add your file processing logic here
      auto err = gplay_.OpenMidi(std::move(data));
      if (!err.empty()) {
        qDebug() << std::format("err={}", err);
        QMessageBox::warning(this, "ModiMidi Warning", 
          qFormat("OpenMidi({}) failed:\n{}", fileName.toStdString(), err));
      }
    }
    playAction->setEnabled(err.empty());
    pauseAction->setEnabled(err.empty());
  }
}

void MainWindow::reOpenFile() {
  if (!lastOpenedPath.isEmpty()) {
      // Logic to reload the file at lastOpenedPath
  }
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
