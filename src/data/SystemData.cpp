#include "data/SystemData.h"
#include <sstream>
#include <vector>
#include <string>
#include <Windows.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <lm.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "netapi32.lib")

SystemInfo::SystemInfo()
    : memorySize(0), diskSize(0) {
    // Get current timestamp
    SYSTEMTIME st;
    GetSystemTime(&st);
    char timestamp[64];
    sprintf_s(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    this->timestamp = timestamp;
}

SystemDataCollector::SystemDataCollector() = default;

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

std::string SystemDataCollector::GetComputerName() {
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    
    if (GetComputerNameA(computerName, &size)) {
        return std::string(computerName, size);
    }
    return "Unknown";
}

std::string SystemDataCollector::GetUserName() {
    char userName[256];
    DWORD size = sizeof(userName);
    
    if (GetUserNameA(userName, &size)) {
        return std::string(userName, size - 1);
    }
    return "Unknown";
}

std::string SystemDataCollector::GetOSVersion() {
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
}

uint64_t SystemDataCollector::GetMemorySize() {
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    
    if (GlobalMemoryStatusEx(&memoryStatus)) {
        return memoryStatus.ullTotalPhys / (1024 * 1024); // MB
    }
    return 0;
}

std::string SystemDataCollector::GetProcessorInfo() {
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
}

uint64_t SystemDataCollector::GetDiskSize() {
    ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
    
    if (GetDiskFreeSpaceExA("C:", &freeBytes, &totalBytes, &totalFreeBytes)) {
        return totalBytes.QuadPart / (1024 * 1024 * 1024); // GB
    }
    return 0;
}

std::string SystemDataCollector::GetNetworkInfo() {
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
}

std::vector<std::string> SystemDataCollector::GetRunningProcesses() {
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