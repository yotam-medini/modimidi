#include "ui.h"
#include "mainwin.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  // 1. Create Actions
  openAction = new QAction(tr("&Open"), this);
  reOpenAction = new QAction(tr("&Re-Open"), this);
  reOpenAction->setEnabled(false); // Disable until a file is opened
  quitAction = new QAction(tr("&Quit"), this);

  // 2. Setup File Menu
  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(openAction);
  fileMenu->addAction(reOpenAction);
  fileMenu->addAction(quitAction);

  QMenu *options_menu = menuBar()->addMenu(tr("&Options"));

#if 0
  // 3. Setup Tool Bar
  QToolBar *fileToolBar = addToolBar(tr("File"));
  fileToolBar->addAction(openAction);
  fileToolBar->addAction(reOpenAction);
#endif

  // 4. Connect Signals
  connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
  connect(reOpenAction, &QAction::triggered, this, &MainWindow::reOpenFile);
  connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void MainWindow::openFile() {
  QString filter = "MIDI Files (*.mid *.MID *.midi *.MIDI);;All Files (*)";

  QString fileName = QFileDialog::getOpenFileName(this,
      tr("Open MIDI File"), "", filter);

  if (!fileName.isEmpty()) {
      lastOpenedPath = fileName;
      reOpenAction->setEnabled(true);
      // Add your file processing logic here
  }
}

void MainWindow::reOpenFile() {
  if (!lastOpenedPath.isEmpty()) {
      // Logic to reload the file at lastOpenedPath
  }
}

class UI::Impl {
 public:
  Impl(int argc, char **argv, bool is_android) :
      argc_{argc},
      argv_{argv},
      app_{argc_, argv_} {

    window_.setWindowTitle("ModiMidi");
    if (is_android) {
      window_.showMaximized();
    } else {
      window_.resize(400, 300);
    }

#if 0
    // Create Toolbar
    QToolBar *toolBar = window_.addToolBar("Main Toolbar");

    // Create Actions
    QAction *fileAction = toolBar->addAction("File");
    QAction *optionsAction = toolBar->addAction("Options");
    toolBar->addSeparator();
    QAction *exitAction = toolBar->addAction("Exit");

    // Central Widget Label
    QLabel *label = new QLabel("Welcome", &window_);
    label->setAlignment(Qt::AlignCenter);
    window_.setCentralWidget(label);

    // Connect Actions (Lambda Functions)
    QObject::connect(fileAction, &QAction::triggered, [label]() {
        label->setText("File Action Clicked");
    });

    QObject::connect(optionsAction, &QAction::triggered, [label]() {
        label->setText("Options Action Clicked");
    });

    QObject::connect(
      exitAction, &QAction::triggered, app_, &QApplication::quit);
#endif
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

UI::UI(int argc, char **argv, bool is_android) :
  impl_{std::make_unique<Impl>(argc, argv, is_android)} {
}

UI::~UI() {
}

int UI::Run() {
  int rc = impl_->Run();
  return rc;
}
