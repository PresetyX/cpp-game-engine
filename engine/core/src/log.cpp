#include "engine/core/log.hpp"

#include <cstdio>
#include <mutex>
#include <utility>

namespace engine {

namespace {

std::mutex g_mutex;
LogLevel g_level = LogLevel::Info;
LogSink g_sink;

const char* level_name(LogLevel level) {
    switch (level) {
    case LogLevel::Trace:
        return "TRACE";
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    }
    return "?";
}

}  // namespace

void set_log_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_level = level;
}

LogLevel log_level() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_level;
}

void set_log_sink(LogSink sink) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_sink = std::move(sink);
}

void log_message(LogLevel level, std::string_view message) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (level < g_level) {
        return;
    }
    if (g_sink) {
        g_sink(level, message);
        return;
    }
    std::fprintf(stderr, "[%s] %.*s\n", level_name(level), static_cast<int>(message.size()),
                 message.data());
}

}  // namespace engine
