#include "ui.h"
#include "mainwin.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QMainWindow>
#include <QProxyStyle>

#include "gplay.h"

class TouchStyle : public QProxyStyle {
 public:
  using QProxyStyle::QProxyStyle;

  int pixelMetric(PixelMetric metric,
                   const QStyleOption *option = nullptr,
                   const QWidget *widget = nullptr) const override {
    if (metric == PM_ScrollBarExtent && widget) {
      // "Container" = the scroll area/viewport this scrollbar belongs to.
      // Use whichever dimension makes sense for how it's mounted; width()
      // covers the common case of a vertical scrollbar at the panel edge.
      const QWidget *container = widget->parentWidget()
                                      ? widget->parentWidget() : widget;
      int extent = container->width() / 12;
      return qBound(24, extent, 64);  // sane touch-target floor/ceiling
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
  }
};

class UI::Impl {
 public:
  Impl(int argc, char **argv, GPlay &gplay, bool is_android) :
      argc_{argc},
      argv_{argv},
      app_{argc_, argv_},
      window_{gplay} {

    window_.setWindowTitle("ModiMidi");
    if (is_android) {
      app_.setStyle(new TouchStyle(app_.style()));
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
