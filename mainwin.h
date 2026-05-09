#pragma once

#include <QAction>
#include <QMainWindow>
#include <QString>

class GPlay;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(GPlay& gplay);

private slots:
  void openFile();
  void reOpenFile();
  void showDebugDialog();

private:
  GPlay &gplay_;
  QString lastOpenedPath;
  QAction *openAction;
  QAction *reOpenAction;
  QAction *showDebugAction;
  QAction *quitAction;
  QAction *pauseAction;
  QAction *stopAction;
  QAction *playAction;
  QAction *forwardAction;
  QAction *backwardAction;
};
