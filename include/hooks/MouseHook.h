#ifndef MOUSEHOOK_H
#define MOUSEHOOK_H

#include "data/KeyData.h"
#include <string>
#include <windows.h>

class DataManager;

enum class MouseButton { LEFT, RIGHT, MIDDLE, X1, X2, NONE };

struct MouseHookData {
    std::string timestamp;
    POINT position;
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
    
    static HHOOK s_mouseHook;
    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    void ProcessMouseEvent(WPARAM eventType, MSLLHOOKSTRUCT* mouseStruct);
    MouseButton GetMouseButton(WPARAM eventType, DWORD mouseData) const;
    void LogMouseEvent(const MouseHookData& mouseData) const;
    std::string GetActiveWindowTitle() const;
    
    static MouseHook* s_instance;
};

#endif // MOUSEHOOK_H