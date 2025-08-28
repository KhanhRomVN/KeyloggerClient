// src/utils/NetworkUtils.cpp
#include "utils/NetworkUtils.h"
#include "core/Logger.h"
#include <Windows.h>
#include <iphlpapi.h>
#include <wlanapi.h>
#include <vector>
#include <string>
#include <cstdint>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wlanapi.lib")

namespace utils {

bool NetworkUtils::CheckInternetConnection() {
    // Simple internet connection check
    return IsOnLocalNetwork(); // For now, assume local network means internet
}

bool NetworkUtils::IsOnLocalNetwork() {
    std::string gateway = GetLocalGateway();
    return !gateway.empty() && IsPrivateIP(gateway);
}

std::string NetworkUtils::GetLocalGateway() {
    PIP_ADAPTER_ADDRESSES adapterAddresses = nullptr;
    ULONG outBufLen = 15000; // 15KB initial buffer
    DWORD dwRetVal = 0;
    std::string gateway;

    // Get the required buffer size
    adapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (!adapterAddresses) {
        LOG_ERROR("Memory allocation failed for IP_ADAPTER_ADDRESSES struct");
        return "";
    }

    // Get adapter addresses
    dwRetVal = GetAdaptersAddresses(AF_INET, 
                                   GAA_FLAG_INCLUDE_GATEWAYS, 
                                   nullptr, 
                                   adapterAddresses, 
                                   &outBufLen);
    
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        free(adapterAddresses);
        adapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        if (!adapterAddresses) {
            LOG_ERROR("Memory allocation failed for IP_ADAPTER_ADDRESSES struct");
            return "";
        }
        dwRetVal = GetAdaptersAddresses(AF_INET, 
                                       GAA_FLAG_INCLUDE_GATEWAYS, 
                                       nullptr, 
                                       adapterAddresses, 
                                       &outBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES adapter = adapterAddresses;
        while (adapter) {
            // Check for active Ethernet or WiFi adapters
            if ((adapter->IfType == IF_TYPE_ETHERNET_CSMACD || 
                 adapter->IfType == IF_TYPE_IEEE80211) && 
                adapter->OperStatus == IfOperStatusUp) {
                
                PIP_ADAPTER_GATEWAY_ADDRESS gatewayAddress = adapter->FirstGatewayAddress;
                if (gatewayAddress) {
                    SOCKET_ADDRESS* sa = &gatewayAddress->Address;
                    if (sa->lpSockaddr->sa_family == AF_INET) {
                        sockaddr_in* sockaddrIpv4 = (sockaddr_in*)sa->lpSockaddr;
                        char ipStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &(sockaddrIpv4->sin_addr), ipStr, INET_ADDRSTRLEN);
                        gateway = ipStr;
                        break;
                    }
                }
            }
            adapter = adapter->Next;
        }
    } else {
        LOG_ERROR("GetAdaptersAddresses failed with error: " + std::to_string(dwRetVal));
    }

    free(adapterAddresses);
    return gateway;
}

std::string NetworkUtils::GetCurrentSSID() {
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    DWORD dwResult = 0;
    PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;
    std::string ssid;

    // Open WLAN client handle
    dwResult = WlanOpenHandle(dwMaxClient, nullptr, &dwCurVersion, &hClient);
    if (dwResult != ERROR_SUCCESS) {
        LOG_ERROR("WlanOpenHandle failed: " + std::to_string(dwResult));
        return "";
    }

    // Enumerate interfaces
    dwResult = WlanEnumInterfaces(hClient, nullptr, &pIfList);
    if (dwResult == ERROR_SUCCESS && pIfList) {
        for (DWORD i = 0; i < pIfList->dwNumberOfItems; i++) {
            PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];
            
            if (pIfInfo->isState == wlan_interface_state_connected) {
                PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = nullptr;
                DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
                WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

                dwResult = WlanQueryInterface(hClient, 
                                            &pIfInfo->InterfaceGuid,
                                            wlan_intf_opcode_current_connection,
                                            nullptr,
                                            &connectInfoSize,
                                            (PVOID*)&pConnectInfo,
                                            &opCode);

                if (dwResult == ERROR_SUCCESS && pConnectInfo) {
                    if (pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength > 0) {
                        ssid.assign((char*)pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID,
                                   pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength);
                        WlanFreeMemory(pConnectInfo);
                        break;
                    }
                    WlanFreeMemory(pConnectInfo);
                }
            }
        }
        WlanFreeMemory(pIfList);
    }

    WlanCloseHandle(hClient, nullptr);
    return ssid;
}

std::vector<std::string> NetworkUtils::GetNetworkAdapters() {
    std::vector<std::string> adapters;
    PIP_ADAPTER_INFO adapterInfo = nullptr;
    ULONG ulOutBufLen = 0;
    DWORD dwRetVal = 0;

    // Get the required buffer size
    dwRetVal = GetAdaptersInfo(adapterInfo, &ulOutBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        adapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        if (!adapterInfo) {
            LOG_ERROR("Memory allocation failed for IP_ADAPTER_INFO");
            return adapters;
        }

        dwRetVal = GetAdaptersInfo(adapterInfo, &ulOutBufLen);
        if (dwRetVal == NO_ERROR) {
            PIP_ADAPTER_INFO adapter = adapterInfo;
            while (adapter) {
                std::string adapterDesc = adapter->Description;
                std::string adapterIp = adapter->IpAddressList.IpAddress.String;
                if (!adapterIp.empty() && adapterIp != "0.0.0.0") {
                    adapters.push_back(adapterDesc + " - " + adapterIp);
                }
                adapter = adapter->Next;
            }
        }
        free(adapterInfo);
    }

    return adapters;
}

bool NetworkUtils::IsPrivateIP(const std::string& ip) {
    // Check for private IP ranges
    if (ip.find("192.168.") == 0) return true;
    if (ip.find("10.") == 0) return true;
    if (ip.find("172.16.") == 0) return true;
    if (ip.find("172.17.") == 0) return true;
    if (ip.find("172.18.") == 0) return true;
    if (ip.find("172.19.") == 0) return true;
    if (ip.find("172.20.") == 0) return true;
    if (ip.find("172.21.") == 0) return true;
    if (ip.find("172.22.") == 0) return true;
    if (ip.find("172.23.") == 0) return true;
    if (ip.find("172.24.") == 0) return true;
    if (ip.find("172.25.") == 0) return true;
    if (ip.find("172.26.") == 0) return true;
    if (ip.find("172.27.") == 0) return true;
    if (ip.find("172.28.") == 0) return true;
    if (ip.find("172.29.") == 0) return true;
    if (ip.find("172.30.") == 0) return true;
    if (ip.find("172.31.") == 0) return true;
    if (ip.find("127.0.0.1") == 0) return true;
    if (ip.find("::1") == 0) return true; // IPv6 localhost
    
    return false;
}

} // namespace utils