#ifndef KEYDATA_H
#define KEYDATA_H

#include <string>

// Key event types
enum class KeyEventType {
    KEY_DOWN,
    KEY_UP
};

// Key modifiers
enum class KeyModifiers {
    NONE    = 0,
    SHIFT   = 1 << 0,
    CONTROL = 1 << 1,
    ALT     = 1 << 2,
    WIN     = 1 << 3
};

// Enable bitwise operations for KeyModifiers
inline KeyModifiers operator|(KeyModifiers a, KeyModifiers b) {
    return static_cast<KeyModifiers>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(KeyModifiers a, KeyModifiers b) {
    return static_cast<int>(a) & static_cast<int>(b);
}

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
