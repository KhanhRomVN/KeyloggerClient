#include "data/SystemData.h"
#include "core/Logger.h"
#include "utils/TimeUtils.h"
#include "utils/SystemUtils.h"
#include <Windows.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <sstream>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

// Obfuscated strings
constexpr auto OBF_SYSTEM_DATA = OBFUSCATE("SystemData");

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
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    
    if (GetComputerNameA(computerName, &size)) {
        return std::string(computerName, size);
    }
    return "Unknown";
}

std::string SystemDataCollector::GetUserName() const {
    char userName[256];
    DWORD size = sizeof(userName);
    
    if (GetUserNameA(userName, &size)) {
        return std::string(userName, size - 1); // Exclude null terminator
    }
    return "Unknown";
}

std::string SystemDataCollector::GetOSVersion() const {
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    
    if (GetVersionExA((OSVERSIONINFOA*)&osvi)) {
        std::stringstream ss;
        ss << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << "."
           << osvi.dwBuildNumber << " " << osvi.szCSDVersion;
        return ss.str();
    }
    return "Unknown";
}

uint64_t SystemDataCollector::GetMemorySize() const {
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    
    if (GlobalMemoryStatusEx(&memoryStatus)) {
        return memoryStatus.ullTotalPhys / (1024 * 1024); // Return in MB
    }
    return 0;
}

std::string SystemDataCollector::GetProcessorInfo() const {
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

uint64_t SystemDataCollector::GetDiskSize() const {
    ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
    
    if (GetDiskFreeSpaceExA("C:", &freeBytes, &totalBytes, &totalFreeBytes)) {
        return totalBytes.QuadPart / (1024 * 1024 * 1024); // Return in GB
    }
    return 0;
}

std::string SystemDataCollector::GetNetworkInfo() const {
    PIP_ADAPTER_INFO adapterInfo = new IP_ADAPTER_INFO[16];
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO) * 16;
    
    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_SUCCESS) {
        std::stringstream ss;
        PIP_ADAPTER_INFO adapter = adapterInfo;
        
        while (adapter) {
            ss << "Adapter: " << adapter->Description 
               << " IP: " << adapter->IpAddressList.IpAddress.String
               << " MAC: ";
            
            for (UINT i = 0; i < adapter->AddressLength; i++) {
                if (i == adapter->AddressLength - 1)
                    ss << std::hex << (int)adapter->Address[i];
                else
                    ss << std::hex << (int)adapter->Address[i] << "-";
            }
            ss << "; ";
            adapter = adapter->Next;
        }
        
        delete[] adapterInfo;
        return ss.str();
    }
    
    delete[] adapterInfo;
    return "Unknown";
}

std::vector<std::string> SystemDataCollector::GetRunningProcesses() const {
    std::vector<std::string> processes;
    DWORD processesIds[1024], cbNeeded;
    
    if (EnumProcesses(processesIds, sizeof(processesIds), &cbNeeded)) {
        DWORD cProcesses = cbNeeded / sizeof(DWORD);
        
        for (DWORD i = 0; i < cProcesses; i++) {
            if (processesIds[i] != 0) {
                TCHAR processName[MAX_PATH] = TEXT("<unknown>");
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | 
                                            PROCESS_VM_READ, FALSE, processesIds[i]);
                
                if (hProcess) {
                    HMODULE hMod;
                    DWORD cbNeeded;
                    
                    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
                        GetModuleBaseNameA(hProcess, hMod, processName, 
                                         sizeof(processName) / sizeof(TCHAR));
                    }
                }
                
                processes.push_back(processName);
                CloseHandle(hProcess);
            }
        }
    }
    
    return processes;
}