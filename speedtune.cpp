#include "speedtune.h"
#include <format>
#include <QLabel>
#include <QPalette>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "qutil.h"

namespace {

void SetTransparentGroove(QSlider *slider) {
  QPalette pal = slider->palette();
  pal.setColor(QPalette::Highlight, Qt::transparent); // or your desired color
  slider->setPalette(pal);
}

QSlider *CreateSpeedSlider(QWidget *page) {
  QSlider *slider = new QSlider(Qt::Horizontal);
  slider->setMinimum(-0x100);
  slider->setMaximum(+0x100);
  slider->setValue(0);
  SetTransparentGroove(slider);
  return slider;
}

QSlider *CreateTuneSlider(QWidget *page) {
  QSlider *slider = new QSlider(Qt::Horizontal);
  slider->setMinimum(-6);
  slider->setMaximum(6);
  slider->setValue(0);
  slider->setTickInterval(1);
  slider->setTickPosition(QSlider::TicksAbove);
  SetTransparentGroove(slider);
  return slider;
}

} // anonymous

QHBoxLayout* CreateSpeedTuneSection(QWidget *page) {
  QHBoxLayout *speed_tune_layout = new QHBoxLayout();
  QVBoxLayout *speed_layout = new QVBoxLayout();
  QVBoxLayout *tune_layout = new QVBoxLayout();
  QSlider *speed_slider = CreateSpeedSlider(page);
  QLabel *speed_label = new QLabel("Speed: 1x", page);
  speed_layout->addWidget(speed_label);
  speed_layout->addWidget(speed_slider);
  QLabel *tune_label = new QLabel("Half Tone Shift: 0", page);
  QSlider *tune_slider = CreateTuneSlider(page);

  QObject::connect(speed_slider, &QSlider::valueChanged, [speed_label](int value) {
    speed_label->setText(qFormat("speed val: {}", value));
    qDebug() << "Speed Current Value:" << value; 
  });

  QObject::connect(speed_slider, &QSlider::sliderReleased, [speed_label, speed_slider]() {
    speed_label->setText(qFormat("released: speed val: {}", speed_slider->value()));
    qDebug() << "Speed release Value:" << speed_slider->value(); 
  });

  QObject::connect(tune_slider, &QSlider::valueChanged, [tune_label](int value) {
    tune_label->setText(qFormat("Half Tone Shift: {}", value));
    qDebug() << "Current Value:" << value; 
  });

  tune_layout->addWidget(tune_label);
  tune_layout->addWidget(tune_slider);
  speed_tune_layout->addLayout(speed_layout);
  speed_tune_layout->addLayout(tune_layout);
  return speed_tune_layout;
}

