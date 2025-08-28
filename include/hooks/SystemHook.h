#ifndef SYSTEMHOOK_H
#define SYSTEMHOOK_H

#include "data/KeyData.h"
#include "core/Platform.h"
#include <string>

class DataManager;

enum class SystemEventType {
    WINDOW_CREATED,
    WINDOW_DESTROYED,
    APP_ACTIVATED,
    SHELL_ACTIVATED
};

struct SystemEventData {
    std::string timestamp;
    SystemEventType eventType;
#if PLATFORM_WINDOWS
    HWND windowHandle;
#else
    void* windowHandle; // Generic handle for cross-platform
#endif
    std::string windowTitle;
    std::string processName;
    std::string extraInfo;
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
    
    // Linux-specific methods (placeholder for future implementation)
#if PLATFORM_LINUX
    void LinuxEventLoop();
    static void* LinuxEventThread(void* context);
#endif
};

#endif