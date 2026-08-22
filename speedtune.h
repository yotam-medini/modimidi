#pragma once

class QHBoxLayout;
class QWidget;
class GPlay;

QHBoxLayout* CreateSpeedTuneSection(
  QWidget *page,
  GPlay &gplay,
  QWidget*& speedtune_container);
