#ifndef KEYHOOK_H
#define KEYHOOK_H

#include "data/KeyData.h"
#include "core/Platform.h"

class DataManager;

class KeyHook {
public:
    explicit KeyHook(DataManager* dataManager);
    ~KeyHook();
    
    bool InstallHook();
    bool RemoveHook();
    bool IsActive() const { return m_isActive; }
    
private:
    DataManager* m_dataManager;
    bool m_isActive;
    
#if PLATFORM_WINDOWS
    static HHOOK s_keyboardHook;
    static KeyHook* s_instance;
    
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    void ProcessKeyEvent(WPARAM eventType, KBDLLHOOKSTRUCT* kbStruct);
    std::string GetActiveWindowTitle() const;
    KeyModifiers GetModifierKeys() const;
    std::string VirtualKeyCodeToString(UINT vkCode) const;
#endif
    
    void LogKeyEvent(const KeyData& keyData) const;
};

#endif