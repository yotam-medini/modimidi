// -*- c++ -*-
#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include <QString>

template <typename... Args>
QString qFormat(std::format_string<Args...> fmt, Args&&... args) {
  return QString::fromStdString(std::format(fmt, std::forward<Args>(args)...));
}

std::string read_binary_file(const QString &path, std::vector<uint8_t>& data);
std::string GetAndroidSoundFontPath(const char *base_filename);

