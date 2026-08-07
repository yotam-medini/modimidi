#include "speedtune.h"
#include <cmath>
#include <format>
#include <QLabel>
#include <QPalette>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "qutil.h"

namespace {

constexpr int SPEED_SCALE = 0x100;

double SpeedScaleToValue(int scaled_value) {
  constexpr double div_SPEED_SCALE = 1.0 / static_cast<double>(SPEED_SCALE);
  const double p = static_cast<double>(scaled_value) * (2.0 * div_SPEED_SCALE);
  constexpr auto log2 = std::log(2.0);
  // 2^p = (e^(log 2))^p = e^((log 2) p);
  const auto v = std::exp(log2 * p);
  return v;
}

void SetTransparentGroove(QSlider *slider) {
  QPalette pal = slider->palette();
  pal.setColor(QPalette::Highlight, Qt::transparent); // or your desired color
  slider->setPalette(pal);
}

QSlider *CreateSpeedSlider(QWidget *page) {
  QSlider *slider = new QSlider(Qt::Horizontal);
  slider->setMinimum(-SPEED_SCALE);
  slider->setMaximum(SPEED_SCALE);
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
  QLabel *speed_label = new QLabel("Speed: x 1.0", page);
  speed_layout->addWidget(speed_label);
  speed_layout->addWidget(speed_slider);
  QLabel *tune_label = new QLabel("Half Tone Shift: 0", page);
  QSlider *tune_slider = CreateTuneSlider(page);

  QObject::connect(speed_slider, &QSlider::valueChanged, [speed_label](int value) {
    const auto x_speed = SpeedScaleToValue(value);
    qDebug() << qFormat("Speed Current Value: {} -> {}", value, x_speed);
    speed_label->setText(qFormat("Speed: x {:5.3}", x_speed));
  });

  QObject::connect(speed_slider, &QSlider::sliderReleased, [speed_label, speed_slider]() {
    const auto value = speed_slider->value();
    const auto x_speed = SpeedScaleToValue(value);
    qDebug() << qFormat("released: speed val: {} -> {}", value, x_speed);
    speed_label->setText(qFormat("Speed: x {:5.3f}", x_speed));
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

