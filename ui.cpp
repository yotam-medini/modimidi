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
    int result;
    if (metric == PM_ScrollBarExtent && widget) {
      // "Container" = the scroll area/viewport this scrollbar belongs to.
      // Use whichever dimension makes sense for how it's mounted; width()
      // covers the common case of a vertical scrollbar at the panel edge.
      const QWidget *container = widget->parentWidget()
                                      ? widget->parentWidget() : widget;
      int extent = container->width() / 12;
      result = qBound(24, extent, 64);  // sane touch-target bounds
    } else if (metric == PM_SplitterWidth && widget) {
      // Qt queries this with 'widget' being the QSplitter itself (see
      // QSplitter::handleWidth()), not the handle -- use its own size.
      // min() so this behaves for horizontal and vertical splitters.
      int reference = qMin(widget->width(), widget->height());
      if (reference <= 0) {
        reference = qMax(widget->width(), widget->height());
      }
      int extent = reference / 40;
      result = qBound(20, extent, 48);
    } else {
      result = QProxyStyle::pixelMetric(metric, option, widget);
    }
    return result;
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
      // showMaximized() still conceptually coexists with system chrome
      // (status bar), which is a common source of a fixed touch-offset
      // bug on Android (touch coordinates vs. assumed window geometry
      // disagreeing by roughly the status bar's height). showFullScreen()
      // gives Qt the whole display surface consistently.
      window_.showFullScreen();
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
