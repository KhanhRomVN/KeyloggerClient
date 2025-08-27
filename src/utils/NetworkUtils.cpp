// src/utils/NetworkUtils.cpp
#include "utils/NetworkUtils.h"
#include <Windows.h>
#include <iphlpapi.h>
#include <vector>
#include <cstdint>
#include <string>

#pragma comment(lib, "iphlpapi.lib")

bool NetworkUtils::IsOnLocalNetwork() {
    std::string gateway = GetLocalGateway();
    
    // Check if gateway is in common private IP ranges
    if (gateway.find("192.168.") == 0 ||
        gateway.find("10.") == 0 ||
        gateway.find("172.16.") == 0 ||
        gateway.find("172.17.") == 0 ||
        gateway.find("172.18.") == 0 ||
        gateway.find("172.19.") == 0 ||
        gateway.find("172.20.") == 0 ||
        gateway.find("172.21.") == 0 ||
        gateway.find("172.22.") == 0 ||
        gateway.find("172.23.") == 0 ||
        gateway.find("172.24.") == 0 ||
        gateway.find("172.25.") == 0 ||
        gateway.find("172.26.") == 0 ||
        gateway.find("172.27.") == 0 ||
        gateway.find("172.28.") == 0 ||
        gateway.find("172.29.") == 0 ||
        gateway.find("172.30.") == 0 ||
        gateway.find("172.31.") == 0) {
        return true;
    }
    
    return false;
}

std::string NetworkUtils::GetLocalGateway() {
    PIP_ADAPTER_INFO adapterInfo;
    PIP_ADAPTER_INFO adapter = NULL;
    DWORD dwRetVal = 0;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    
    adapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
    if (adapterInfo == NULL) {
        return "";
    }
    
    if (GetAdaptersInfo(adapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(adapterInfo);
        adapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
        if (adapterInfo == NULL) {
            return "";
        }
    }
    
    if ((dwRetVal = GetAdaptersInfo(adapterInfo, &ulOutBufLen)) == NO_ERROR) {
        adapter = adapterInfo;
        while (adapter) {
            if (adapter->Type == MIB_IF_TYPE_ETHERNET || 
                adapter->Type == IF_TYPE_IEEE80211) { // Ethernet or WiFi
                return adapter->GatewayList.IpAddress.String;
            }
            adapter = adapter->Next;
        }
    }
    
    free(adapterInfo);
    return "";
}