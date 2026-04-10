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

private:
  GPlay &gplay_;
  QString lastOpenedPath;
  QAction *openAction;
  QAction *reOpenAction;
  QAction *quitAction;
  QAction *playAction;
  QAction *pauseAction;
};
