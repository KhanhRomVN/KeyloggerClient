#ifndef KEYDATA_H
#define KEYDATA_H

#include <string>
#include <windows.h>

// Key event types
enum class KeyEventType {
    KEY_DOWN,
    KEY_UP
};

// Key modifiers
enum class KeyModifiers {
    NONE = 0,
    SHIFT = 1 << 0,
    CONTROL = 1 << 1,
    ALT = 1 << 2,
    WIN = 1 << 3,
    CAPS_LOCK = 1 << 4,
    NUM_LOCK = 1 << 5
};

// Enable bitwise operations for KeyModifiers
inline KeyModifiers operator|(KeyModifiers a, KeyModifiers b) {
    return static_cast<KeyModifiers>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline KeyModifiers operator&(KeyModifiers a, KeyModifiers b) {
    return static_cast<KeyModifiers>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline KeyModifiers& operator|=(KeyModifiers& a, KeyModifiers b) {
    a = a | b;
    return a;
}

inline bool HasModifier(KeyModifiers modifiers, KeyModifiers test) {
    return (static_cast<uint32_t>(modifiers) & static_cast<uint32_t>(test)) != 0;
}

// Windows virtual key codes
enum class WindowsKey : int {
    KEY_SHIFT    = VK_SHIFT,
    KEY_CONTROL  = VK_CONTROL,
    KEY_MENU     = VK_MENU,     // ALT key
    KEY_LWIN     = VK_LWIN,
    KEY_RWIN     = VK_RWIN,
    KEY_CAPITAL  = VK_CAPITAL,
    KEY_TAB      = VK_TAB,
    KEY_RETURN   = VK_RETURN,
    KEY_ESCAPE   = VK_ESCAPE,
    KEY_SPACE    = VK_SPACE,
    KEY_BACK     = VK_BACK,
    KEY_DELETE   = VK_DELETE
};

class KeyData {
public:
    KeyData();

    [[nodiscard]] bool IsModifierKey() const;
    [[nodiscard]] std::string ToString() const;

    std::string timestamp;
    int keyCode;
    int scanCode;
    int flags;
    KeyEventType eventType;
    KeyModifiers modifiers;
    std::string keyName;
    std::string windowTitle;
};

#endif // KEYDATA_H