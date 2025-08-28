#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>    

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static void Init(const std::string& logPath);
    static void Shutdown();
    
    static void Write(LogLevel level, const std::string& message);
    
    static void SetLogLevel(LogLevel level) { m_logLevel = level; }
    static LogLevel GetLogLevel() { return m_logLevel; }
    
    static void RotateLogFile();
    static void EncryptLogs();
    
private:
    static std::ofstream m_logFile;
    static std::string m_logPath;
    static std::mutex m_mutex;
    static LogLevel m_logLevel;
    
    static std::string LogLevelToString(LogLevel level);
};

// Helper macros
#define LOG_DEBUG(message) Logger::Write(LogLevel::DEBUG, message)
#define LOG_INFO(message) Logger::Write(LogLevel::INFO, message)
#define LOG_WARN(message) Logger::Write(LogLevel::WARN, message)
#define LOG_ERROR(message) Logger::Write(LogLevel::ERROR, message)

#endif