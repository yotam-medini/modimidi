#pragma once

#include <array>
#include <QMainWindow>
#include <QString>

class GPlay;
class QAction;
class QToolButton;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(GPlay& gplay);

 private slots:
  void openFile();
  void reOpenFile();
  void showDebugDialog();

 private:
  enum {Pause, Stop, Play, Forward, Backward, N_ButtonOps};
  GPlay &gplay_;
  QString lastOpenedPath;
  QAction *openAction;
  QAction *reOpenAction;
  QAction *showDebugAction;
  QAction *quitAction;
#if 0
  QAction *pauseAction;
  QAction *stopAction;
  QAction *playAction;
  QAction *forwardAction;
  QAction *backwardAction;
#endif
  std::array<QAction*, N_ButtonOps> actions_;
  std::array<QToolButton*, N_ButtonOps> buttons_;
};
