#ifndef KEYHOOK_H
#define KEYHOOK_H

#include "data/KeyData.h"
#include <windows.h>

class DataManager;

class KeyHook {
public:
    explicit KeyHook(DataManager* dataManager);
    ~KeyHook();
    
    bool InstallHook();
    bool RemoveHook();
    
private:
    DataManager* m_dataManager;
    bool m_isActive;
    static HHOOK s_keyboardHook;
    static KeyHook* s_instance;
    
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    void ProcessKeyEvent(WPARAM eventType, KBDLLHOOKSTRUCT* kbStruct);
    std::string GetActiveWindowTitle() const;
    KeyModifiers GetModifierKeys() const;
    std::string VirtualKeyCodeToString(UINT vkCode) const;
    void LogKeyEvent(const KeyData& keyData) const;
};

#endif