#ifndef PRIVILEGEESCALATION_H
#define PRIVILEGEESCALATION_H

#include "core/Platform.h"
#include <string>

class PrivilegeEscalation {
public:
    static bool EnablePrivilege(const char* privilege);
    static bool IsAdmin();
    static bool TryUACBypass();
    static bool CreateElevatedProcess();
    static bool InjectIntoProcess(uint32_t pid);
    static bool ModifyTokenPrivileges();
    static bool StealToken(uint32_t pid);
    
private:
#if PLATFORM_WINDOWS
    static bool EnableWindowsPrivilege(LPCTSTR privilege);
    static bool IsWindowsAdmin();
    static bool TryWindowsUACBypass();
    static bool CreateWindowsElevatedProcess();
    static bool InjectIntoWindowsProcess(DWORD pid);
    static bool ModifyWindowsTokenPrivileges();
    static bool StealWindowsToken(DWORD pid);
#endif
};

#endif