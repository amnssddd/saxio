#pragma once
#include <iostream>
#include <string>
#include <mutex>
#include <format>
#include <ctime>
#include <vector>

namespace saxio::log {
class Logger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    // 设置日志级别
    void setLevel(Level level) {
        std::lock_guard lock(mutex_);
        loglevel_ = level;
    }

    // 带颜色的日志级别字符串（静态版本天生线程安全）
    static std::string level_to_string(const Level level) {
        switch (level) {
            case Level::DEBUG:   return "\033[36m[DEBUG]\033[0m";   // 青色
            case Level::INFO:    return "\033[32m[INFO]\033[0m";    // 绿色
            case Level::WARNING: return "\033[33m[WARNING]\033[0m"; // 黄色
            case Level::ERROR:   return "\033[31m[ERROR]\033[0m";   // 红色
            default:             return "[UNKNOWN]";
        }
    }

    // 可变参数日志方法（核心改进）
    template <typename... Args>
    void log(Level level, std::format_string<Args...> fmt, Args&&... args) {
        std::lock_guard lock(mutex_);

        // 手动格式化时间
        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);  //转换为C风格时间
        char time_buf[64];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %X", std::localtime(&time));

        // 使用 std::format 组合日志
        std::string message = std::format(
            "{} {} {}",  // 格式字符串
            time_buf,     // 时间
            level_to_string(level),
            std::format(fmt, std::forward<Args>(args)...)
        );

        std::cout << message << std::endl;
    }

    // 具体日志级别方法
    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(Level::DEBUG, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) {
        log(Level::INFO, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(std::format_string<Args...> fmt, Args&&... args) {
        log(Level::WARNING, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) {
        log(Level::ERROR, fmt, std::forward<Args>(args)...);
    }

private:
    Level loglevel_{Level::DEBUG};
    std::mutex mutex_;
};
} // namespace saxio::log