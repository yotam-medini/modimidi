#pragma once
#include <format>
#include <string>
#include <QString>

template <>
struct std::formatter<QString> : std::formatter<std::string> {
    auto format(QString const& s, std::format_context& ctx) const {
        return std::formatter<std::string>::format(s.toStdString(), ctx);
    }
};
