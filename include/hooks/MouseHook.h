#ifndef MOUSEHOOK_H
#define MOUSEHOOK_H

#include "data/KeyData.h"
#include "core/Platform.h"
#include <string>

#if PLATFORM_WINDOWS
#include <windows.h>
#endif

class DataManager;

enum class MouseButton { LEFT, RIGHT, MIDDLE, X1, X2, NONE };

struct MouseHookData {
    std::string timestamp;
#if PLATFORM_WINDOWS
    POINT position;
#else
    struct Point {
        int x;
        int y;
    } position;
#endif
    unsigned int eventType;
    unsigned long mouseData;
    unsigned long flags;
    MouseButton button;
    int wheelDelta;
};

class MouseHook {
public:
    explicit MouseHook(DataManager* dataManager);
    ~MouseHook();
    
    bool InstallHook();
    bool RemoveHook();
    
private:
    DataManager* m_dataManager;
    bool m_isActive;
    
#if PLATFORM_WINDOWS
    static HHOOK s_mouseHook;
    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    void ProcessMouseEvent(WPARAM eventType, MSLLHOOKSTRUCT* mouseStruct);
    MouseButton GetMouseButton(WPARAM eventType, DWORD mouseData) const;
#elif PLATFORM_LINUX
    static void* MouseThread(void* param);
    void* m_mouseThread;
    bool m_threadRunning;
#endif
    
    static MouseHook* s_instance;
    void LogMouseEvent(const MouseHookData& mouseData) const;
};

#endif // MOUSEHOOK_H