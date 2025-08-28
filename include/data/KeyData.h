#ifndef KEYDATA_H
#define KEYDATA_H

#include "core/Platform.h"
#include <string>
#include <cstdint>

// Key event types
enum class KeyEventType {
    KEY_DOWN,
    KEY_UP
};

// Key modifiers
enum class KeyModifiers : uint32_t {
    NONE    = 0,
    SHIFT   = 1 << 0,
    CONTROL = 1 << 1,
    ALT     = 1 << 2,
    WIN     = 1 << 3
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
enum class VirtualKey : int {
    VK_SHIFT    = 0x10,
    VK_CONTROL  = 0x11,
    VK_MENU     = 0x12,     // ALT key
    VK_LWIN     = 0x5B,
    VK_RWIN     = 0x5C,
    VK_CAPITAL  = 0x14,
    VK_TAB      = 0x09,
    VK_RETURN   = 0x0D,
    VK_ESCAPE   = 0x1B,
    VK_SPACE    = 0x20,
    VK_BACK     = 0x08,
    VK_DELETE   = 0x2E
};

class KeyData {
public:
    KeyData();

    bool IsModifierKey() const;
    std::string ToString() const;

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