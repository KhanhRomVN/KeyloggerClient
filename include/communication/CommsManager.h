#ifndef COMMSMANAGER_H
#define COMMSMANAGER_H

#include <memory>
#include <unordered_map>
#include <string>

class Configuration;
class BaseComms;

class CommsManager {
public:
    explicit CommsManager(Configuration* config);
    ~CommsManager();
    
    bool Initialize();
    bool TransmitData(const std::vector<uint8_t>& data);
    void Shutdown();
    bool TestConnection() const;
    
private:
    Configuration* m_config;
    BaseComms* m_currentMethod;
    std::unordered_map<std::string, std::unique_ptr<BaseComms>> m_commsMethods;
    
    void InitializeCommsMethods();
    bool TryFallbackMethods(const std::string& failedMethod);
    void RotateCommsMethod();
};

#endif