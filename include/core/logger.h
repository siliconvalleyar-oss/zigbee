#pragma once

#include "types.h"
#include <string>
#include <memory>
#include <fstream>
#include <mutex>
#include <chrono>
#include <cstdio>
#include <cstdarg>
#include <array>

namespace zigbee_mesh::core {

enum class LogLevel : uint8_t {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5,
    Off = 6,
};

enum class LogSink : uint8_t {
    Console = 0,
    File = 1,
    Both = 2,
};

struct LoggerConfig {
    LogLevel level{LogLevel::Info};
    LogSink sink{LogSink::Console};
    std::string log_file{"zigbee_mesh.log"};
    size_t max_file_size{10 * 1024 * 1024};
    size_t max_files{5};
    bool colored{true};
    std::string timestamp_format{"%Y-%m-%d %H:%M:%S.%f"};
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    bool init(const LoggerConfig& config = LoggerConfig{}) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;

        if (config_.sink == LogSink::File || config_.sink == LogSink::Both) {
            file_stream_.open(config_.log_file, std::ios::app);
            if (!file_stream_.is_open()) {
                fprintf(stderr, "[Logger] Failed to open log file: %s\n",
                        config_.log_file.c_str());
                return false;
            }
        }
        initialized_ = true;
        return true;
    }

    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.level = level;
    }

    LogLevel getLevel() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.level;
    }

    void setSink(LogSink sink) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.sink = sink;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
        initialized_ = false;
    }

    void log(LogLevel level, const char* file, int line, const char* fmt, ...) {
        if (level < config_.level || !initialized_) return;

        std::lock_guard<std::mutex> lock(mutex_);

        char msg_buf[4096];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
        va_end(args);

        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        struct tm tm_buf;
        localtime_r(&time_t_now, &tm_buf);

        char ts_buf[64];
        snprintf(ts_buf, sizeof(ts_buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                 tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                 tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec,
                 static_cast<int>(ms.count()));

        const char* level_str = levelToString(level);

        const char* basename = file;
        for (const char* p = file; *p; ++p) {
            if (*p == '/' || *p == '\\') basename = p + 1;
        }

        if (config_.sink == LogSink::Console || config_.sink == LogSink::Both) {
            if (config_.colored && isatty(fileno(stderr))) {
                const char* color = colorForLevel(level);
                fprintf(stderr, "%s[%s]%s %s [%s:%d] %s\n",
                        color, ts_buf, "\033[0m", level_str, basename, line, msg_buf);
            } else {
                fprintf(stderr, "[%s] %s [%s:%d] %s\n",
                        ts_buf, level_str, basename, line, msg_buf);
            }
        }

        if ((config_.sink == LogSink::File || config_.sink == LogSink::Both) &&
            file_stream_.is_open()) {
            file_stream_ << "[" << ts_buf << "] " << level_str
                         << " [" << basename << ":" << line << "] "
                         << msg_buf << "\n";
            file_stream_.flush();
        }
    }

private:
    Logger() = default;
    ~Logger() { shutdown(); }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Trace: return "TRACE";
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO ";
            case LogLevel::Warn:  return "WARN ";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
            default:              return "?????";
        }
    }

    static const char* colorForLevel(LogLevel level) {
        switch (level) {
            case LogLevel::Trace: return "\033[37m";
            case LogLevel::Debug: return "\033[36m";
            case LogLevel::Info:  return "\033[32m";
            case LogLevel::Warn:  return "\033[33m";
            case LogLevel::Error: return "\033[31m";
            case LogLevel::Fatal: return "\033[35m";
            default:              return "\033[0m";
        }
    }

    mutable std::mutex mutex_;
    LoggerConfig config_;
    std::ofstream file_stream_;
    bool initialized_{false};
};

} // namespace zigbee_mesh::core

#define ZIGBEE_LOG_TRACE(fmt, ...) \
    zigbee_mesh::core::Logger::getInstance().log( \
        zigbee_mesh::core::LogLevel::Trace, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define ZIGBEE_LOG_DEBUG(fmt, ...) \
    zigbee_mesh::core::Logger::getInstance().log( \
        zigbee_mesh::core::LogLevel::Debug, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define ZIGBEE_LOG_INFO(fmt, ...) \
    zigbee_mesh::core::Logger::getInstance().log( \
        zigbee_mesh::core::LogLevel::Info, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define ZIGBEE_LOG_WARN(fmt, ...) \
    zigbee_mesh::core::Logger::getInstance().log( \
        zigbee_mesh::core::LogLevel::Warn, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define ZIGBEE_LOG_ERROR(fmt, ...) \
    zigbee_mesh::core::Logger::getInstance().log( \
        zigbee_mesh::core::LogLevel::Error, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define ZIGBEE_LOG_FATAL(fmt, ...) \
    zigbee_mesh::core::Logger::getInstance().log( \
        zigbee_mesh::core::LogLevel::Fatal, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
