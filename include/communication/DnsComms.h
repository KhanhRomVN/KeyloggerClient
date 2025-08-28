#ifndef DNSCOMMS_H
#define DNSCOMMS_H

#include "communication/BaseComms.h"
#include "core/Platform.h"
#include <string>

class Configuration;

class DnsComms : public BaseComms { 
public:
    explicit DnsComms(Configuration* config);
    ~DnsComms();    
    
    bool Initialize() override;
    bool SendData(const std::vector<uint8_t>& data) override;
    void Cleanup() override;
    bool TestConnection() const override;
    std::vector<uint8_t> ReceiveData() override;
    
private:
    Configuration* m_config;
    std::string m_dnsServer;
    
    // Platform-specific DNS query methods
    bool SendDataWindows(const std::vector<std::string>& chunks) const;
    bool SendDataLinux(const std::vector<std::string>& chunks) const;
    bool TestConnectionWindows() const;
    bool TestConnectionLinux() const;
};

#endif