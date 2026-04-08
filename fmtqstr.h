#pragma once
#include <format>
#include <string>
#include <QString>

template <>
struct std::formatter<QString> : std::formatter<std::string> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return std::formatter<std::string>::parse(ctx);
    }

    auto format(const QString& s, std::format_context& ctx) const {
        return std::formatter<std::string>::format(s.toStdString(), ctx);
    }
};
