// src/utils/SystemUtils.cpp
#include "utils/SystemUtils.h"
#include "core/Logger.h"
#include "core/Platform.h"
#include <sstream>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <string>
#include <cctype>
#include <cstdio>

#include "StringUtils.h"
#include "SystemData.h"

#if PLATFORM_WINDOWS
#include <Lmcons.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <wininet.h>
#include <Windows.h>
#include <winreg.h>
#include <sid.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "advapi32.lib")
#elif PLATFORM_LINUX
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <pwd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>
#include <fstream>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace utils {

std::string SystemUtils::GetComputerName() {
#if PLATFORM_WINDOWS
    char buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(buffer);
    
    if (::GetComputerNameA(buffer, &size)) {
        return std::string(buffer, size);
    }
    return "Unknown";
#elif PLATFORM_LINUX
    char buffer[256];
    if (gethostname(buffer, sizeof(buffer)) == 0) {
        return std::string(buffer);
    }
    return "Unknown";
#endif
}

std::string SystemUtils::GetUserName() {
#if PLATFORM_WINDOWS
    char buffer[UNLEN + 1];
    DWORD size = sizeof(buffer);
    
    if (::GetUserNameA(buffer, &size)) {
        return std::string(buffer, size - 1); // Exclude null terminator
    }
    return "Unknown";
#elif PLATFORM_LINUX
    struct passwd *pwd = getpwuid(getuid());
    if (pwd) {
        return std::string(pwd->pw_name);
    }
    
    // Fallback to environment variable
    const char* username = getenv("USER");
    if (username) {
        return std::string(username);
    }
    
    return "Unknown";
#endif
}

std::string SystemUtils::GetOSVersion() {
#if PLATFORM_WINDOWS
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    
    if (GetVersionExA((OSVERSIONINFOA*)&osvi)) {
        return StringUtils::Format("%d.%d.%d %s", 
            osvi.dwMajorVersion, osvi.dwMinorVersion, 
            osvi.dwBuildNumber, osvi.szCSDVersion);
    }
    return "Unknown";
#elif PLATFORM_LINUX
    struct utsname uts;
    if (uname(&uts) == 0) {
        return utils::StringUtils::Format("%s %s %s", uts.sysname, uts.release, uts.machine);
    }
    return "Unknown";
#endif
}

uint64_t SystemUtils::GetMemorySize() {
#if PLATFORM_WINDOWS
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    
    if (GlobalMemoryStatusEx(&memoryStatus)) {
        return memoryStatus.ullTotalPhys;
    }
    return 0;
#elif PLATFORM_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.totalram * info.mem_unit;
    }
    return 0;
#endif
}

std::string SystemUtils::GetProcessorInfo() {
#if PLATFORM_WINDOWS
    HKEY hKey;
    char processorName[256];
    DWORD size = sizeof(processorName);
    
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                     "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                     0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL,
                            (LPBYTE)processorName, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(processorName);
        }
        RegCloseKey(hKey);
    }
    return "Unknown";
#elif PLATFORM_LINUX
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string model = line.substr(colon + 1);
                return StringUtils::Trim(model);
            }
        }
    }
    
    return "Unknown";
#endif
}

std::string SystemUtils::GetSystemFingerprint() {
    std::string fingerprint = GetComputerName() + GetUserName() + GetProcessorInfo();
    return fingerprint;
}

bool SystemUtils::IsElevated() {
#if PLATFORM_WINDOWS
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(&ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup)) {
        return false;
    }

    if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
        isAdmin = FALSE;
    }

    FreeSid(adminGroup);
    return isAdmin;
#elif PLATFORM_LINUX
    return geteuid() == 0;
#endif
}

bool SystemUtils::EnableStealthMode() {
    // Implement various stealth techniques
    return true;
}

bool SystemUtils::IsExitTriggered() {
    // Check for exit conditions like specific files or registry keys
    return false;
}

SystemInfo SystemUtils::CollectSystemInformation() {
    SystemInfo info;
    info.computerName = GetComputerName();
    info.userName = GetUserName();
    info.osVersion = GetOSVersion();
    info.memorySize = GetMemorySize();
    info.processorInfo = GetProcessorInfo();
    // Initialize other fields
    info.diskSize = 0;
    info.networkInfo = "";
    info.runningProcesses = GetRunningProcesses();
    info.timestamp = "";
    return info;
}

std::vector<std::string> SystemUtils::GetRunningProcesses() {
    std::vector<std::string> processes;
    
#if PLATFORM_WINDOWS
    DWORD processesIds[1024], cbNeeded;
    
    if (EnumProcesses(processesIds, sizeof(processesIds), &cbNeeded)) {
        DWORD cProcesses = cbNeeded / sizeof(DWORD);
        
        for (DWORD i = 0; i < cProcesses; i++) {
            if (processesIds[i] != 0) {
                char processName[MAX_PATH] = "<unknown>";
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | 
                                            PROCESS_VM_READ, FALSE, processesIds[i]);
                
                if (hProcess) {
                    HMODULE hMod;
                    DWORD cbNeeded;
                    
                    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
                        GetModuleBaseNameA(hProcess, hMod, processName, 
                                         sizeof(processName));
                    }
                    CloseHandle(hProcess);
                }
                
                processes.push_back(processName);
            }
        }
    }
#elif PLATFORM_LINUX
    // Linux implementation using /proc filesystem
    DIR* procDir = opendir("/proc");
    if (procDir) {
        struct dirent* entry;
        while ((entry = readdir(procDir)) != NULL) {
            // Only consider numeric directories (process IDs)
            if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
                std::string cmdPath = std::string("/proc/") + entry->d_name + "/cmdline";
                std::ifstream cmdFile(cmdPath);
                std::string processName;
                
                if (std::getline(cmdFile, processName)) {
                    // Extract just the executable name
                    size_t slashPos = processName.find_last_of('/');
                    if (slashPos != std::string::npos) {
                        processName = processName.substr(slashPos + 1);
                    }
                    
                    // Remove null characters that might be present in cmdline
                    processName.erase(std::remove(processName.begin(), processName.end(), '\0'), processName.end());
                    
                    if (!processName.empty()) {
                        processes.push_back(processName);
                    }
                }
            }
        }
        closedir(procDir);
    }
#endif
    
    return processes;
}

bool SystemUtils::CheckInternetConnection() {
#if PLATFORM_WINDOWS
    return InternetCheckConnectionA("http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0);
#elif PLATFORM_LINUX
    // Try to connect to a well-known DNS server
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }
    
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(53); // DNS port
    inet_pton(AF_INET, "8.8.8.8", &server.sin_addr); // Google DNS
    
    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    bool connected = (connect(sock, (struct sockaddr*)&server, sizeof(server)) == 0);
    close(sock);
    
    return connected;
#endif
}

std::string SystemUtils::GetMACAddress() {
#if PLATFORM_WINDOWS
    PIP_ADAPTER_INFO adapterInfo = new IP_ADAPTER_INFO[16];
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO) * 16;
    
    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO adapter = adapterInfo;
        if (adapter) {
            char mac[18];
            snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                    adapter->Address[0], adapter->Address[1],
                    adapter->Address[2], adapter->Address[3],
                    adapter->Address[4], adapter->Address[5]);
            
            delete[] adapterInfo;
            return mac;
        }
    }
    
    delete[] adapterInfo;
    return "Unknown";
#elif PLATFORM_LINUX
    struct ifaddrs *ifaddr, *ifa;
    char mac[18] = "Unknown";
    
    if (getifaddrs(&ifaddr) == -1) {
        return mac;
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_PACKET) {
            struct sockaddr_ll *s = (struct sockaddr_ll*)ifa->ifa_addr;
            
            // Skip loopback interface
            if (strcmp(ifa->ifa_name, "lo") == 0) {
                continue;
            }
            
            if (s->sll_halen == 6) { // Ensure we have a valid MAC address
                snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                        s->sll_addr[0], s->sll_addr[1], s->sll_addr[2],
                        s->sll_addr[3], s->sll_addr[4], s->sll_addr[5]);
                break;
            }
        }
    }
    
    freeifaddrs(ifaddr);
    return mac;
#endif
}

void SystemUtils::CriticalShutdown() {
    // Emergency cleanup and shutdown procedure
    Platform::ExitProcess(0);
}
} // namespace utils