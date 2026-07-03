#pragma once

#include <array>
#include <QMainWindow>
#include <QString>
#include "state.h"

class GPlay;
class QAction;
class QLabel;
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
  struct Op {
    enum {Pause, Stop, Play, Forward, Backward, N_ButtonOps};
  };
  void OnStateChange(State state);
  void SetButtonOpColor(size_t button_op_index);
  void ConnectButtonsActions();

  QWidget *BuildPlayerPage();
  QWidget *BuildModifyPage();
  QWidget *BuildInfoPage();
  QWidget *BuildAboutPage();

  GPlay &gplay_;

  QLabel* fileLabel_{nullptr};

  QString lastOpenedPath;
  QAction *openAction;
  QAction *reOpenAction{nullptr};
  QAction *showDebugAction;
  QAction *quitAction;
  std::array<QAction*, Op::N_ButtonOps> actions_;
  std::array<QToolButton*, Op::N_ButtonOps> buttons_;

  QTabWidget* tabs_{nullptr};
};
