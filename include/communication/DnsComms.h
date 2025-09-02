#ifndef DNSCOMMS_H
#define DNSCOMMS_H

#include "communication/BaseComms.h"
#include <string>
#include <vector>

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
    
    [[nodiscard]] bool SendDataInternal(const std::vector<std::string>& chunks) const;
    [[nodiscard]] bool TestConnectionInternal() const;
};

#endif