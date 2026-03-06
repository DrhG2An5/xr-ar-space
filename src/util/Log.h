#pragma once

#include <iostream>
#include <string_view>
#include <format>
#include <chrono>

namespace xr {

class Log {
public:
    enum class Level { Debug, Info, Warn, Error };

    static void setLevel(Level level) { s_level = level; }

    template<typename... Args>
    static void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(Level::Debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args) {
        log(Level::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warn(std::format_string<Args...> fmt, Args&&... args) {
        log(Level::Warn, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args) {
        log(Level::Error, fmt, std::forward<Args>(args)...);
    }

private:
    static inline Level s_level = Level::Info;

    static const char* levelStr(Level l) {
        switch (l) {
            case Level::Debug: return "DEBUG";
            case Level::Info:  return "INFO ";
            case Level::Warn:  return "WARN ";
            case Level::Error: return "ERROR";
        }
        return "?????";
    }

    template<typename... Args>
    static void log(Level level, std::format_string<Args...> fmt, Args&&... args) {
        if (level < s_level) return;
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &time);

        auto& out = (level >= Level::Warn) ? std::cerr : std::cout;
        out << std::format("[{:02d}:{:02d}:{:02d}.{:03d}] [{}] ",
            tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<int>(ms.count()),
            levelStr(level));
        out << std::format(fmt, std::forward<Args>(args)...) << std::endl;
    }
};

} // namespace xr
