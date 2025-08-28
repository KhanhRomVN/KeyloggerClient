#include "core/Logger.h"
#include "utils/TimeUtils.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "security/Encryption.h"
#include <iostream>
#include <sstream>
#include <iomanip>  
#include <vector>
#include <cstdint>
#include <string>

// Windows specific includes
#if PLATFORM_WINDOWS
#include <Windows.h>
#endif

// Static member initialization
std::ofstream Logger::m_logFile;
std::string Logger::m_logPath;
std::mutex Logger::m_mutex;
LogLevel Logger::m_logLevel = LogLevel::INFO;

void Logger::Init(const std::string& logPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_logPath = logPath;
    
    // Convert to wide string for Windows file operations if needed
    std::wstring wideLogPath = utils::StringUtils::Utf8ToWide(logPath);
    utils::FileUtils::CreateDirectories(utils::FileUtils::GetDirectoryPath(wideLogPath));
    
    m_logFile.open(logPath, std::ios::out | std::ios::app);
    if (!m_logFile.is_open()) {
        throw std::runtime_error("Failed to open log file");
    }

    LOG_INFO("Logger initialized successfully");
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        LOG_INFO("Logger shutting down");
        m_logFile.close();
    }
}

void Logger::Write(LogLevel level, const std::string& message) {
    if (level < m_logLevel) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto timestamp = utils::TimeUtils::GetCurrentTimestamp();
    std::string levelStr = LogLevelToString(level);
    
    std::stringstream logEntry;
    logEntry << "[" << timestamp << "] "
             << "[" << levelStr << "] "
             << message << std::endl;

    // Console output (debug builds only)
#ifdef _DEBUG
    std::cout << logEntry.str();
#endif

    // File output
    if (m_logFile.is_open()) {
        m_logFile << logEntry.str();
        m_logFile.flush();
    }

    // Emergency output if file logging fails (Windows only)
    if (!m_logFile.is_open() && level >= LogLevel::ERROR) {
#if PLATFORM_WINDOWS
        OutputDebugStringA(logEntry.str().c_str()); // Convert to const char*
#endif
    }
}

std::string Logger::LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void Logger::RotateLogFile() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_logFile.is_open()) {
        m_logFile.close();
    }

    // Create timestamped backup
    std::string backupPath = m_logPath + "." + 
        utils::TimeUtils::GetCurrentTimestamp(true);
    
    // Convert paths to wide strings for Windows
    std::wstring wideLogPath = utils::StringUtils::Utf8ToWide(m_logPath);
    std::wstring wideBackupPath = utils::StringUtils::Utf8ToWide(backupPath);
    
    utils::FileUtils::MoveFile(wideLogPath, wideBackupPath);
    
    // Reopen log file
    m_logFile.open(m_logPath, std::ios::out | std::ios::app);
}

void Logger::EncryptLogs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_logFile.is_open()) {
        m_logFile.close();
    }

    try {
        // Convert path to wide string for file operations
        std::wstring wideLogPath = utils::StringUtils::Utf8ToWide(m_logPath);
        auto logData = utils::FileUtils::ReadBinaryFile(wideLogPath);
        
        if (!logData.empty()) {
            // Convert vector<uint8_t> to string for encryption
            std::string logDataStr(logData.begin(), logData.end());
            
            // Encrypt the log data
            std::vector<uint8_t> encryptedData = security::Encryption::EncryptAES(
                logData,
                "LOG_ENCRYPTION_KEY_4F2A9C" // Use actual key management in production
            );
            
            // Write encrypted file
            std::wstring encryptedPath = wideLogPath + L".enc";
            utils::FileUtils::WriteBinaryFile(encryptedPath, encryptedData);
            
            // Delete original log file
            utils::FileUtils::DeleteFile(wideLogPath);
        }
    }
    catch (...) {
        // Restart logging if encryption fails
        m_logFile.open(m_logPath, std::ios::out | std::ios::app);
    }
}

// Helper functions implementation
void LogDebug(const std::string& message) {
    Logger::Write(LogLevel::DEBUG, message);
}

void LogInfo(const std::string& message) {
    Logger::Write(LogLevel::INFO, message);
}

void LogWarning(const std::string& message) {
    Logger::Write(LogLevel::WARN, message);
}

void LogError(const std::string& message) {
    Logger::Write(LogLevel::ERROR, message);
}