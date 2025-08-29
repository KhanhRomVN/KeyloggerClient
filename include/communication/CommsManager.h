#ifndef COMMSMANAGER_H
#define COMMSMANAGER_H

#include <memory>   
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

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
    
    // Security layer methods
    std::vector<uint8_t> ApplySecurityLayers(const std::vector<uint8_t>& data) const;

    static std::vector<uint8_t> AddMetadata(const std::vector<uint8_t>& data);
    static void AddIntegrityCheck(std::vector<uint8_t>& data);
    std::vector<uint8_t> ObfuscateData(const std::vector<uint8_t>& data) const;
    static void AddStealthHeaders(std::vector<uint8_t>& data);
};

#endif // COMMSMANAGER_H