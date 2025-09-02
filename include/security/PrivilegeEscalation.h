#ifndef PRIVILEGEESCALATION_H
#define PRIVILEGEESCALATION_H

#include <Windows.h>
#include <string>

class PrivilegeEscalation {
public:
    static bool EnablePrivilege(const char* privilege);
    static bool IsAdmin();
    static bool TryUACBypass();
    static bool CreateElevatedProcess();
    static bool InjectIntoProcess(DWORD pid);
    static bool ModifyTokenPrivileges();
    static bool StealToken(DWORD pid);
};

#endif