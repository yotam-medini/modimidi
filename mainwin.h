#pragma once

#include <QAction>
#include <QMainWindow>
#include <QString>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);

private slots:
  void openFile();
  void reOpenFile();

private:
  QString lastOpenedPath;
  QAction *openAction;
  QAction *reOpenAction;
  QAction *quitAction;
};
