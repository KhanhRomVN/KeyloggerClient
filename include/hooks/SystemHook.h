// include/hooks/SystemHook.h
#ifndef SYSTEMHOOK_H
#define SYSTEMHOOK_H

#include "core/Platform.h"
#include "data/DataManager.h" // Add this include
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
    
#if PLATFORM_WINDOWS
    static HHOOK s_systemHook;
#endif
    
    static SystemHook* s_instance;
    
#if PLATFORM_WINDOWS
    static LRESULT CALLBACK ShellProc(int nCode, WPARAM wParam, LPARAM lParam); 
    void ProcessShellEvent(WPARAM eventType, LPARAM lParam);
    void HandleWindowCreated(HWND hwnd);
    void HandleWindowDestroyed(HWND hwnd);
    void HandleAppActivated(HWND hwnd);
    std::string GetWindowTitle(HWND hwnd) const;
    std::string GetProcessName(HWND hwnd) const;
#endif
    
    void HandleShellActivated();
    
#if PLATFORM_LINUX
    void LinuxEventLoop();
    static void* LinuxEventThread(void* context);
#endif
};

} // namespace hooks

#endif