#include "ui.h"
#include "mainwin.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QMainWindow>

#include "gplay.h"

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
      // Android's launcher icon comes from android/res/mipmap-*/
      // (see AndroidManifest.xml's android:icon);n
    } else {
      window_.resize(400, 300);
      // ":/icon.png" is icon.png (lily.d/, generated from icon.ly)
      // embedded as a Qt resource by CMakeLists.txt.
      const QIcon icon(":/icon.png");
      app_.setWindowIcon(icon);
      window_.setWindowIcon(icon);
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
