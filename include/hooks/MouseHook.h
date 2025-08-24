#ifndef MOUSEHOOK_H
#define MOUSEHOOK_H

#include "data/KeyData.h"
#include <windows.h>

class DataManager;

enum class MouseButton { LEFT, RIGHT, MIDDLE, X1, X2, NONE };

struct MouseData {
    std::string timestamp;
    POINT position;
    UINT eventType;
    DWORD mouseData;
    DWORD flags;
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
    static MouseHook* s_instance;
    
    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    void ProcessMouseEvent(WPARAM eventType, MSLLHOOKSTRUCT* mouseStruct);
    MouseButton GetMouseButton(WPARAM eventType, DWORD mouseData) const;
    void LogMouseEvent(const MouseData& mouseData) const;
};

#endif