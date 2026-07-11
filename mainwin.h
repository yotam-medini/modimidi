#pragma once

#include <array>
#include <QMainWindow>
#include <QString>
#include "state.h"

class GPlay;
class QAction;
class QLabel;
class QPushButton;
class QToolButton;
class RangeSlider;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(GPlay& gplay);

 private slots:
  void openFile();
  void reOpenFile();
  void showDebugDialog();
  void showFilePathDialog();
  void confirmQuit();
  void editRangeStart();
  void editRangeEnd();

 private:
  struct Op {
    enum {Pause, Stop, Play, Forward, Backward, N_ButtonOps};
  };
  void OnStateChange(State state);
  void SetButtonOpColor(size_t button_op_index);
  void ConnectButtonsActions();

  // Shared implementation for editRangeStart()/editRangeEnd(): pops
  // up a "MM:SS:mmm" text-input dialog, validates it, and applies it
  // to the RangeSlider on success.
  void EditRangeValue(bool is_start);

  void UpdateRangeStartLabel(uint32_t ms);
  void UpdateRangeEndLabel(uint32_t ms);

  QWidget *BuildPlayerPage();
  QWidget *BuildModifyPage();
  QWidget *BuildInfoPage();
  QWidget *BuildAboutPage();

  GPlay &gplay_;

  QPushButton* fileButton_{nullptr};
  QString lastOpenedPath;
  QAction *openAction;
  QAction *reOpenAction{nullptr};
  QAction *showDebugAction;
  QAction *quitAction;

  // Container for rangeSlider_ + the start/end label-buttons, so the
  // whole group can be shown/hidden as one unit (hidden until a
  // proper midi file has been loaded).
  QWidget* rangeGroup_{nullptr};
  RangeSlider* rangeSlider_{nullptr};
  // "Start: MM:SS.mmm" / "End: MM:SS.mmm"; clicking either pops up a
  // MM:SS:mmm text-input dialog to set that value exactly.
  QPushButton* rangeStartLabel_{nullptr};
  QPushButton* rangeEndLabel_{nullptr};

  std::array<QAction*, Op::N_ButtonOps> actions_;
  std::array<QToolButton*, Op::N_ButtonOps> buttons_;

  QTabWidget* tabs_{nullptr};
};
