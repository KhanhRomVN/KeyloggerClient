#ifndef KEYDATA_H
#define KEYDATA_H

#include "core/Platform.h"
#include <string>

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

// Cross-platform virtual key codes
enum class PlatformKey : int {
    KEY_SHIFT    = 0x10,
    KEY_CONTROL  = 0x11,
    KEY_MENU     = 0x12,     // ALT key
    KEY_LWIN     = 0x5B,
    KEY_RWIN     = 0x5C,
    KEY_CAPITAL  = 0x14,
    KEY_TAB      = 0x09,
    KEY_RETURN   = 0x0D,
    KEY_ESCAPE   = 0x1B,
    KEY_SPACE    = 0x20,
    KEY_BACK     = 0x08,
    KEY_DELETE   = 0x2E, VK_SHIFT, VK_CONTROL, VK_MENU, VK_LWIN, VK_RWIN, VK_CAPITAL
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