#pragma once

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>

namespace engine {

enum class LogLevel : std::uint8_t { Trace = 0, Debug, Info, Warn, Error };

using LogSink = std::function<void(LogLevel, std::string_view)>;

// Messages below this level are discarded. Default: Info.
void set_log_level(LogLevel level);
LogLevel log_level();

// Replaces the output sink. An empty sink restores the default (stderr).
void set_log_sink(LogSink sink);

// Sends an already formatted message through the filter and the sink.
void log_message(LogLevel level, std::string_view message);

// Streams all arguments into one message: log(LogLevel::Info, "x=", 3).
template <typename... Args>
void log(LogLevel level, const Args&... args) {
    if (level < log_level()) {
        return;
    }
    std::ostringstream stream;
    (stream << ... << args);
    log_message(level, stream.str());
}

template <typename... Args>
void log_debug(const Args&... args) {
    log(LogLevel::Debug, args...);
}
template <typename... Args>
void log_info(const Args&... args) {
    log(LogLevel::Info, args...);
}
template <typename... Args>
void log_warn(const Args&... args) {
    log(LogLevel::Warn, args...);
}
template <typename... Args>
void log_error(const Args&... args) {
    log(LogLevel::Error, args...);
}

}  // namespace engine
