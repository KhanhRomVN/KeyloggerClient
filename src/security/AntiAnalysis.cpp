#include "security/AntiAnalysis.h"
#include "core/Logger.h"
#include "utils/SystemUtils.h"
#include "utils/FileUtils.h"
#include "utils/TimeUtils.h"
#include "core/Platform.h"

#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <cstdint>
#include <thread>
#include <random>

#if PLATFORM_WINDOWS
#include <Windows.h>
#include <winreg.h>
#include <intrin.h>
#elif PLATFORM_LINUX
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <fcntl.h>
#include <cpuid.h>
#endif

namespace security {

bool AntiAnalysis::IsDebuggerPresent() {
#if PLATFORM_WINDOWS
    std::vector<bool(*)(void)> checks = {
        []() -> bool { return !!::IsDebuggerPresent(); },
        []() -> bool {
            HMODULE hModule = GetModuleHandleA("kernel32.dll");
            return hModule && GetProcAddress(hModule, "IsDebuggerPresent");
        },
        []() -> bool {
            __try {
                __asm { int 3 }
                return false;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return true;
            }
        },
        []() -> bool {
            HANDLE hProcess = GetCurrentProcess();
            BOOL isDebugged = FALSE;
            return CheckRemoteDebuggerPresent(hProcess, &isDebugged) && isDebugged;
        }
    };

    for (const auto& check : checks) {
        if (check()) {
            LOG_WARN("Debugger detected");
            return true;
        }
    }
#elif PLATFORM_LINUX
    // Linux debugger detection
    std::ifstream status("/proc/self/status");
    std::string line;
    
    while (std::getline(status, line)) {
        if (line.find("TracerPid:") == 0) {
            std::string pidStr = line.substr(10);
            pidStr.erase(0, pidStr.find_first_not_of(" \t"));
            int tracerPid = std::stoi(pidStr);
            if (tracerPid != 0) {
                LOG_WARN("Debugger detected (TracerPid: " + std::to_string(tracerPid) + ")");
                return true;
            }
        }
    }
    
    // Check via ptrace
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) != -1) {
        LOG_WARN("Process is being traced (debugged)");
        return true;
    }
#endif
    
    return false;
}

bool AntiAnalysis::IsRunningInVM() {
    std::vector<bool> results;
    
#if PLATFORM_WINDOWS
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
    
    // CPUID-based check for Windows
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
               vendor.find("KVM") != std::string::npos;
    }());

#elif PLATFORM_LINUX
    // Linux-specific VM checks
    results.push_back([]() -> bool {
        // Check /proc/cpuinfo for hypervisor flag
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        
        while (std::getline(cpuinfo, line)) {
            if (line.find("hypervisor") != std::string::npos) {
                return true;
            }
        }
        return false;
    }());
    
    // Check dmesg for virtualization messages
    results.push_back([]() -> bool {
        FILE* pipe = popen("dmesg | grep -i hypervisor 2>/dev/null", "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string output(buffer);
                if (output.find("hypervisor") != std::string::npos) {
                    pclose(pipe);
                    return true;
                }
            }
            pclose(pipe);
        }
        return false;
    }());
#endif

    // Cross-platform VM checks
    results.push_back([]() -> bool {
        // Check for common VM files
        const char* vmFiles[] = {
            "/.dockerenv", // Docker
            "/.dockerinit", // Docker
            "/proc/1/cgroup", // Container check
        };
        
        for (const auto& file : vmFiles) {
            struct stat buffer;
            if (stat(file, &buffer) == 0) {
                return true;
            }
        }
        return false;
    }());

    for (bool result : results) {
        if (result) {
            LOG_WARN("Virtual machine/container detected");
            return true;
        }
    }
    
    return false;
}

bool AntiAnalysis::IsSandboxed() {
    std::vector<bool> results;
    
    // Check for small amount of memory
    results.push_back([]() -> bool {
#if PLATFORM_WINDOWS
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        GlobalMemoryStatusEx(&memStatus);
        return memStatus.ullTotalPhys < (2ULL * 1024 * 1024 * 1024); // Less than 2GB
#elif PLATFORM_LINUX
        struct sysinfo info;
        if (sysinfo(&info) == 0) {
            return info.totalram < (2ULL * 1024 * 1024 * 1024); // Less than 2GB
        }
        return false;
#endif
    }());

    // Check for recent system uptime
    results.push_back([]() -> bool {
#if PLATFORM_WINDOWS
        return GetTickCount64() < (2 * 60 * 60 * 1000); // Less than 2 hours
#elif PLATFORM_LINUX
        struct sysinfo info;
        if (sysinfo(&info) == 0) {
            return info.uptime < (2 * 60 * 60); // Less than 2 hours
        }
        return false;
#endif
    }());

    // Check for sandbox-specific artifacts
    results.push_back([]() -> bool {
        const char* sandboxPaths[] = {
            "/analysis",
            "/sandbox",
            "/malware",
            "/sample",
            "C:\\analysis",
            "C:\\sandbox", 
            "C:\\malware",
            "C:\\sample"
        };

        for (const auto& path : sandboxPaths) {
#if PLATFORM_WINDOWS
            if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
                return true;
            }
#elif PLATFORM_LINUX
            struct stat buffer;
            if (stat(path, &buffer) == 0) {
                return true;
            }
#endif
        }
        return false;
    }());

    for (bool result : results) {
        if (result) {
            LOG_WARN("Sandbox environment detected");
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
#if PLATFORM_WINDOWS
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);

    return memStatus.dwMemoryLoad > 90 || // Memory usage > 90%
           memStatus.ullAvailPhys < (512 * 1024 * 1024); // Less than 512MB available
#elif PLATFORM_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        float memoryUsage = 100.0f * (info.totalram - info.freeram) / info.totalram;
        return memoryUsage > 90.0f || // Memory usage > 90%
               info.freeram < (512 * 1024 * 1024); // Less than 512MB available
    }
    return false;
#endif
}

void AntiAnalysis::Countermeasure() {
    if (IsDebuggerPresent()) {
        LOG_WARN("Debugger detected, activating countermeasures");
        ExecuteDecoyOperations();
    }

    if (IsRunningInVM() || IsSandboxed()) {
        LOG_WARN("Analysis environment detected, altering behavior");
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
    std::string decoyFile = tempPath + "system_cache.tmp";

    std::ofstream file(decoyFile);
    if (file.is_open()) {
        file << "System Cache File\n";
        file << "This file is used by system processes for temporary storage\n";
        file.close();
        
        // Make file hidden on Windows
#if PLATFORM_WINDOWS
        SetFileAttributesA(decoyFile.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
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