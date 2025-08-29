#include "utils/NetworkUtils.h"
#include "core/Platform.h"
#include "core/Logger.h"
#include <vector>
#include <string>

#if PLATFORM_WINDOWS
    #include <iphlpapi.h>
    #include <wlanapi.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "wlanapi.lib")
#elif PLATFORM_LINUX
    #include <fstream>
    #include <sstream>
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <net/if.h>
    #include <ifaddrs.h>
#endif

namespace utils {

bool NetworkUtils::CheckInternetConnection() {
    // Simple internet connection check - try to resolve a well-known domain
    #if PLATFORM_WINDOWS
        struct hostent* host = gethostbyname("www.google.com");
        return host != nullptr;
    #elif PLATFORM_LINUX
        struct hostent* host = gethostbyname("www.google.com");
        return host != nullptr;
    #endif
}

bool NetworkUtils::IsOnLocalNetwork() {
    std::string gateway = GetLocalGateway();
    return !gateway.empty() && IsPrivateIP(gateway);
}

std::string NetworkUtils::GetLocalGateway() {
    std::string gateway;
    
    #if PLATFORM_WINDOWS
        PIP_ADAPTER_ADDRESSES adapterAddresses = nullptr;
        ULONG outBufLen = 15000;
        DWORD dwRetVal = 0;

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
        
    #elif PLATFORM_LINUX
        // Read default gateway from /proc/net/route
        std::ifstream routeFile("/proc/net/route");

    if (routeFile.is_open()) {
        std::string line;
        // Skip header line
            std::getline(routeFile, line);
            
            while (std::getline(routeFile, line)) {
                std::istringstream iss(line);
                std::string iface, destination, gw, flags;
                iss >> iface >> destination >> gw >> flags;
                
                // Check if this is the default route (destination 00000000)
                if (destination == "00000000" && flags.find("0003") != std::string::npos) {
                    // Convert hex gateway to IP address
                    if (gw.size() == 8) {
                        std::string byte1 = gw.substr(6, 2);
                        std::string byte2 = gw.substr(4, 2);
                        std::string byte3 = gw.substr(2, 2);
                        std::string byte4 = gw.substr(0, 2);
                        
                        gateway = std::to_string(std::stoi(byte1, nullptr, 16)) + "." +
                                 std::to_string(std::stoi(byte2, nullptr, 16)) + "." +
                                 std::to_string(std::stoi(byte3, nullptr, 16)) + "." +
                                 std::to_string(std::stoi(byte4, nullptr, 16));
                        break;
                    }
                }
            }
            routeFile.close();
        }
    #endif
    
    return gateway;
}

std::string NetworkUtils::GetCurrentSSID() {
    std::string ssid;
    
    #if PLATFORM_WINDOWS
        HANDLE hClient = NULL;
        DWORD dwMaxClient = 2;
        DWORD dwCurVersion = 0;
        DWORD dwResult = 0;
        PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;

        dwResult = WlanOpenHandle(dwMaxClient, nullptr, &dwCurVersion, &hClient);
        if (dwResult != ERROR_SUCCESS) {
            LOG_ERROR("WlanOpenHandle failed: " + std::to_string(dwResult));
            return "";
        }

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
        
    #elif PLATFORM_LINUX
        // On Linux, we can try to get SSID from iwconfig or nmcli
    if (FILE* pipe = popen("iwgetid -r 2>/dev/null", "r")) {
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                ssid = buffer;
                // Remove trailing newline
                if (!ssid.empty() && ssid[ssid.length()-1] == '\n') {
                    ssid.erase(ssid.length()-1);
                }
            }
            pclose(pipe);
        }
    #endif
    
    return ssid;
}

std::vector<std::string> NetworkUtils::GetNetworkAdapters() {
    std::vector<std::string> adapters;
    
    #if PLATFORM_WINDOWS
        PIP_ADAPTER_INFO adapterInfo = nullptr;
        ULONG ulOutBufLen = 0;
        DWORD dwRetVal = 0;

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
        
    #elif PLATFORM_LINUX
        struct ifaddrs* ifaddr = nullptr;
        if (getifaddrs(&ifaddr) == 0) {
            for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                    char ip[INET_ADDRSTRLEN];
                    auto* sa = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
                    inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
                    
                    if (strcmp(ip, "127.0.0.1") != 0) {
                        adapters.push_back(std::string(ifa->ifa_name) + " - " + ip);
                    }
                }
            }
            freeifaddrs(ifaddr);
        }
    #endif
    
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
    if (ip.find("::1") == 0) return true;
    
    return false;
}

} // namespace utils