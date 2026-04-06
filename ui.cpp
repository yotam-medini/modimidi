#include "ui.h"

#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>

class UI::UiImpl {
 public:
  UiImpl(int argc, char **argv, bool is_android) :
      argc_{argc},
      argv_{argv},
      app_{argc_, argv_} {

    window_.setWindowTitle("Minimal Qt Toolbar");
    window_.resize(400, 300);

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
      exitAction, &QAction::triggered, &app_, &QApplication::quit);
  }

  int Run() {
    window_.show();
    return app_.exec();
  }
 private:
   int argc_;
   char **argv_;
   QApplication app_;
   QMainWindow window_;
};

UI::UI(int argc, char **argv, bool is_android) :
  impl_{std::make_unique<UiImpl>(argc, argv, is_android)} {
}

UI::~UI() {
}

int UI::Run() {
  int rc = impl_->Run();
  return rc;
}
