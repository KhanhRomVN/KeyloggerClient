#include "data/SystemData.h"
#include "core/Logger.h"
#include "utils/TimeUtils.h"
#include "utils/SystemUtils.h"
#include <sstream>
#include <vector>
#include <cstdint>
#include <string>

#if PLATFORM_WINDOWS
    #include <Windows.h>
    #include <iphlpapi.h>
    #include <psapi.h>
    #include <lm.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "psapi.lib")
    #pragma comment(lib, "netapi32.lib")
#elif PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/utsname.h>
    #include <sys/sysinfo.h>
    #include <sys/types.h>
    #include <pwd.h>
    #include <fstream>
    #include <dirent.h>
    #include <ifaddrs.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

SystemInfo::SystemInfo()
    : timestamp(utils::TimeUtils::GetCurrentTimestamp()),
      memorySize(0),
      diskSize(0) {}

SystemDataCollector::SystemDataCollector() {}

SystemInfo SystemDataCollector::Collect() const {
    SystemInfo info;
    
    info.computerName = GetComputerName();
    info.userName = GetUserName();
    info.osVersion = GetOSVersion();
    info.memorySize = GetMemorySize();
    info.processorInfo = GetProcessorInfo();
    info.diskSize = GetDiskSize();
    info.networkInfo = GetNetworkInfo();
    info.runningProcesses = GetRunningProcesses();
    
    return info;
}

std::string SystemDataCollector::GetComputerName() const {
#if PLATFORM_WINDOWS
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    
    if (GetComputerNameA(computerName, &size)) {
        return std::string(computerName, size);
    }
    return "Unknown";
#elif PLATFORM_LINUX
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "Unknown";
#endif
}

std::string SystemDataCollector::GetUserName() const {
#if PLATFORM_WINDOWS
    char userName[256];
    DWORD size = sizeof(userName);
    
    if (GetUserNameA(userName, &size)) {
        return std::string(userName, size - 1);
    }
    return "Unknown";
#elif PLATFORM_LINUX
    struct passwd* pwd = getpwuid(getuid());
    if (pwd && pwd->pw_name) {
        return std::string(pwd->pw_name);
    }
    
    char* envUser = getenv("USER");
    if (envUser) {
        return std::string(envUser);
    }
    
    return "Unknown";
#endif
}

std::string SystemDataCollector::GetOSVersion() const {
#if PLATFORM_WINDOWS
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    
    if (GetVersionExA((OSVERSIONINFOA*)&osvi)) {
        std::stringstream ss;
        ss << "Windows " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion
           << " Build " << osvi.dwBuildNumber;
        return ss.str();
    }
    return "Unknown Windows Version";
#elif PLATFORM_LINUX
    struct utsname uts;
    if (uname(&uts) == 0) {
        std::stringstream ss;
        ss << uts.sysname << " " << uts.release << " " << uts.machine;
        return ss.str();
    }
    return "Unknown Linux Version";
#endif
}

uint64_t SystemDataCollector::GetMemorySize() const {
#if PLATFORM_WINDOWS
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    
    if (GlobalMemoryStatusEx(&memoryStatus)) {
        return memoryStatus.ullTotalPhys / (1024 * 1024); // MB
    }
    return 0;
#elif PLATFORM_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return (info.totalram * info.mem_unit) / (1024 * 1024); // MB
    }
    return 0;
#endif
}

std::string SystemDataCollector::GetProcessorInfo() const {
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
    return "Unknown Processor";
#elif PLATFORM_LINUX
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string name = line.substr(colon + 1);
                // Trim whitespace
                name.erase(0, name.find_first_not_of(" \t"));
                name.erase(name.find_last_not_of(" \t") + 1);
                return name;
            }
        }
    }
    return "Unknown Processor";
#endif
}

uint64_t SystemDataCollector::GetDiskSize() const {
#if PLATFORM_WINDOWS
    ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
    
    if (GetDiskFreeSpaceExA("C:", &freeBytes, &totalBytes, &totalFreeBytes)) {
        return totalBytes.QuadPart / (1024 * 1024 * 1024); // GB
    }
    return 0;
#elif PLATFORM_LINUX
    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
        uint64_t totalBytes = stat.f_blocks * stat.f_frsize;
        return totalBytes / (1024 * 1024 * 1024); // GB
    }
    return 0;
#endif
}

std::string SystemDataCollector::GetNetworkInfo() const {
#if PLATFORM_WINDOWS
    PIP_ADAPTER_INFO adapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO) * 16);
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO) * 16;
    
    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_SUCCESS) {
        std::stringstream ss;
        PIP_ADAPTER_INFO adapter = adapterInfo;
        
        while (adapter) {
            ss << "Adapter: " << adapter->Description 
               << " IP: " << adapter->IpAddressList.IpAddress.String
               << " MAC: ";
            
            for (UINT i = 0; i < adapter->AddressLength; i++) {
                ss << std::hex << (int)adapter->Address[i];
                if (i < adapter->AddressLength - 1) ss << "-";
            }
            ss << "; ";
            adapter = adapter->Next;
        }
        
        free(adapterInfo);
        return ss.str();
    }
    
    free(adapterInfo);
    return "Unknown Network Info";
#elif PLATFORM_LINUX
    std::stringstream ss;
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == 0) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL) continue;
            
            int family = ifa->ifa_addr->sa_family;
            if (family == AF_INET || family == AF_INET6) {
                char host[NI_MAXHOST];
                int s = getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                
                if (s == 0) {
                    ss << "Interface: " << ifa->ifa_name << " IP: " << host << "; ";
                }
            }
        }
        freeifaddrs(ifaddr);
        return ss.str();
    }
    return "Unknown Network Info";
#endif
}

std::vector<std::string> SystemDataCollector::GetRunningProcesses() const {
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
    DIR* dir = opendir("/proc");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            // Check if directory name is a number (process ID)
            if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
                std::string path = std::string("/proc/") + entry->d_name + "/comm";
                std::ifstream commFile(path.c_str());
                std::string processName;
                
                if (commFile) {
                    std::getline(commFile, processName);
                    if (!processName.empty()) {
                        processes.push_back(processName);
                    }
                }
            }
        }
        closedir(dir);
    }
#endif
    
    return processes;
}