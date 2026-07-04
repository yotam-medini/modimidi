#include "qutil.h"
#include <format>
#include <vector>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QString>
#include "debug.h"

std::string read_binary_file(const QString &path, std::vector<uint8_t>& data) {
  std::string error;
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    error = std::format("{}\nOpening {}",
      file.errorString().toStdString(), path.toStdString());
  }
  if (error.empty()) {
    auto fileSize = file.size();
    data.clear();
    data.resize(static_cast<std::size_t>(fileSize));

    qint64 bytesRead = file.read(reinterpret_cast<char*>(
      data.data()), fileSize);
    if (bytesRead != fileSize) {
      data.clear();
      error = std::format("bytesRead={} != {}=fileSize of {}",
        bytesRead, fileSize, path.toStdString());
    }
  }

  return error;
}

#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QString>
#include <format>

std::string GetAndroidSoundFontPath(const char *base_filename) {
  std::string result_path = "";
  const QString writableDir =
    QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(writableDir);
  QString destPath = writableDir + "/" + base_filename;
  QString resPath = QString(":/") + base_filename;

  // 1. Check if it already exists in Android internal storage
  if (QFile::exists(destPath)) {
    result_path = destPath.toStdString();
  } else {
    DebugMessage::AddMessage("SF2 not in storage. Attempting copy...");

    // 2. Verify internal resource existence
    bool existsInApk = QFile::exists(resPath);
    DebugMessage::AddMessage(std::format("Internal Exist? {} : {}",
      existsInApk, resPath.toStdString()));

    if (existsInApk) {
      if (QFile::copy(resPath, destPath)) {
        QFile::setPermissions(
          destPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        DebugMessage::AddMessage("Copy successful.");
        result_path = destPath.toStdString();
      } else {
        DebugMessage::AddMessage(
          std::format("Failed copy to: {}", destPath.toStdString()));
      }
    } else {
      DebugMessage::AddMessage("ERROR: SF2 missing from APK resources!");
    }
  }

  return result_path;
}
