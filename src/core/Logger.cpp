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
LogLevel Logger::m_logLevel = LogLevel::LEVEL_INFO;

void Logger::Init(const std::string& logPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_logPath = logPath;
    
    // Get directory path from the log path
    std::string directoryPath = utils::FileUtils::GetDirectoryPath(logPath);
    
    // Convert to wide string for Windows file operations if needed
    #if PLATFORM_WINDOWS
    std::wstring wideDirectoryPath = utils::StringUtils::Utf8ToWide(directoryPath);
    utils::FileUtils::CreateDirectories(wideDirectoryPath);
    #else
    utils::FileUtils::CreateDirectories(directoryPath);
    #endif
    
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
    if (!m_logFile.is_open() && level >= LogLevel::LEVEL_ERROR) {
#if PLATFORM_WINDOWS
        OutputDebugStringA(logEntry.str().c_str()); // Convert to const char*
#endif
    }
}

std::string Logger::LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::LEVEL_DEBUG: return "DEBUG";
        case LogLevel::LEVEL_INFO:  return "INFO";
        case LogLevel::LEVEL_WARN:  return "WARN";
        case LogLevel::LEVEL_ERROR: return "ERROR";
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
    
    // Use appropriate string types based on platform
    #if PLATFORM_WINDOWS
    // Convert paths to wide strings for Windows
    std::wstring wideLogPath = utils::StringUtils::Utf8ToWide(m_logPath);
    std::wstring wideBackupPath = utils::StringUtils::Utf8ToWide(backupPath);
    utils::FileUtils::MoveFile(wideLogPath, wideBackupPath);
    #else
    utils::FileUtils::MoveFile(m_logPath, backupPath);
    #endif
    
    // Reopen log file
    m_logFile.open(m_logPath, std::ios::out | std::ios::app);
}

void Logger::EncryptLogs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_logFile.is_open()) {
        m_logFile.close();
    }

    try {
        // Read the log file
        #if PLATFORM_WINDOWS
        std::wstring wideLogPath = utils::StringUtils::Utf8ToWide(m_logPath);
        auto logData = utils::FileUtils::ReadBinaryFile(wideLogPath);
        #else
        auto logData = utils::FileUtils::ReadBinaryFile(m_logPath);
        #endif
        
        if (!logData.empty()) {
            // Encrypt the log data
            std::vector<uint8_t> encryptedData = security::Encryption::EncryptAES(
                logData,
                "LOG_ENCRYPTION_KEY_4F2A9C" // Use actual key management in production
            );
            
            // Write encrypted file
            #if PLATFORM_WINDOWS
            std::wstring encryptedPath = wideLogPath + L".enc";
            utils::FileUtils::WriteBinaryFile(encryptedPath, encryptedData);
            
            // Delete original log file
            utils::FileUtils::DeleteFile(wideLogPath);
            #else
            std::string encryptedPath = m_logPath + ".enc";
            utils::FileUtils::WriteBinaryFile(encryptedPath, encryptedData);
            
            // Delete original log file
            utils::FileUtils::DeleteFile(m_logPath);
            #endif
        }
    }
    catch (...) {
        // Restart logging if encryption fails
        m_logFile.open(m_logPath, std::ios::out | std::ios::app);
    }
}

// Helper functions implementation
void LogDebug(const std::string& message) {
    Logger::Write(LogLevel::LEVEL_DEBUG, message);
}

void LogInfo(const std::string& message) {
    Logger::Write(LogLevel::LEVEL_INFO, message);
}

void LogWarning(const std::string& message) {
    Logger::Write(LogLevel::LEVEL_WARN, message);
}

void LogError(const std::string& message) {
    Logger::Write(LogLevel::LEVEL_ERROR, message);
}