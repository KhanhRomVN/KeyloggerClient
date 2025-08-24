// File: Logger.h
#pragma once

#include "utils/TimeUtils.h"
#include "utils/FileUtils.h"
#include "security/Obfuscation.h"
#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>

namespace core {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static Logger& getInstance();

    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Initializes logger with configuration
     * @param logFilePath Path to log file (empty for memory-only)
     * @param maxFileSize Maximum log file size in bytes
     * @param enableObfuscation Whether to obfuscate log entries
     */
    void initialize(const std::string& logFilePath = "", 
                   size_t maxFileSize = 1048576, 
                   bool enableObfuscation = true);

    /**
     * @brief Writes log entry with specified level
     * @param level Log severity level
     * @param module Originating module name
     * @param message Log message content
     */
    void log(LogLevel level, const std::string& module, const std::string& message);

    // Convenience methods for different log levels
    void debug(const std::string& module, const std::string& message);
    void info(const std::string& module, const std::string& message);
    void warning(const std::string& module, const std::string& message);
    void error(const std::string& module, const std::string& message);
    void critical(const std::string& module, const std::string& message);

    /**
     * @brief Flushes all buffered log entries to disk
     */
    void flush();

    /**
     * @brief Retrieves recent log entries from memory buffer
     * @param maxEntries Maximum number of entries to retrieve
     * @return Vector of log entries
     */
    std::vector<std::string> getRecentLogs(size_t maxEntries = 100) const;

    /**
     * @brief Sets minimum log level for filtering
     * @param level Minimum level to log
     */
    void setMinLogLevel(LogLevel level);

    /**
     * @brief Emergency log purge for security purposes
     */
    void emergencyPurge();

private:
    Logger();
    ~Logger();

    /**
     * @brief Background thread function for writing logs
     */
    void processLogQueue();

    /**
     * @brief Rotates log file if size exceeds limit
     */
    void rotateLogFileIfNeeded();

    /**
     * @brief Writes single log entry to destination
     */
    void writeLogEntry(const std::string& logEntry);

    struct LogEntry {
        std::string timestamp;
        LogLevel level;
        std::string module;
        std::string message;
    };

    // Log processing queue
    std::queue<LogEntry> logQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::atomic<bool> shutdownRequested_{false};

    // Log writer thread
    std::thread writerThread_;

    // Log configuration
    std::string logFilePath_;
    size_t maxFileSize_;
    std::atomic<LogLevel> minLogLevel_{LogLevel::INFO};
    std::atomic<bool> enableObfuscation_{true};

    // Output stream
    std::unique_ptr<std::ofstream> logFile_;
    mutable std::mutex fileMutex_;

    // Security components
    std::unique_ptr<security::Obfuscation> obfuscator_;

    // Memory buffer for recent logs
    std::vector<std::string> recentLogs_;
    mutable std::mutex recentLogsMutex_;
    static const size_t MAX_MEMORY_LOGS = 1000;
};

} // namespace core