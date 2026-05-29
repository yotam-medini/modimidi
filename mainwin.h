#pragma once

#include <array>
#include <QMainWindow>
#include <QString>
#include "state.h"

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
  void OnStateChange(State state);
  void SetButtonOpColor(size_t button_op_index);
  GPlay &gplay_;
  QString lastOpenedPath;
  QAction *openAction;
  QAction *reOpenAction;
  QAction *showDebugAction;
  QAction *quitAction;
  std::array<QAction*, N_ButtonOps> actions_;
  std::array<QToolButton*, N_ButtonOps> buttons_;
};
