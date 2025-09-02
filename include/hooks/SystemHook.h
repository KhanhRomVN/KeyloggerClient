#ifndef SYSTEMHOOK_H
#define SYSTEMHOOK_H

#include <windows.h>
#include <string>

class DataManager;

namespace hooks {

enum class SystemEventType {
    WINDOW_CREATED,
    WINDOW_DESTROYED,
    APP_ACTIVATED,
    SHELL_ACTIVATED
};

class SystemHook {
public:
    explicit SystemHook(DataManager* dataManager);
    ~SystemHook();
    
    bool InstallHook();
    bool RemoveHook();
    
private:
    DataManager* m_dataManager;
    bool m_isActive;
    
    static HHOOK s_systemHook;
    static SystemHook* s_instance;
    
    static LRESULT CALLBACK ShellProc(int nCode, WPARAM wParam, LPARAM lParam); 
    void ProcessShellEvent(WPARAM eventType, LPARAM lParam);
    void HandleWindowCreated(HWND hwnd);
    void HandleWindowDestroyed(HWND hwnd);
    void HandleAppActivated(HWND hwnd);
    std::string GetWindowTitle(HWND hwnd) const;
    std::string GetProcessName(HWND hwnd) const;
    
    void HandleShellActivated() const;
};

} // namespace hooks

#endif