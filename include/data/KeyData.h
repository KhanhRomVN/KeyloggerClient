#ifndef KEYDATA_H
#define KEYDATA_H

#include <string>
#include <cstdint>

enum class KeyEventType { KEY_DOWN, KEY_UP };
enum class KeyModifiers {
    NONE = 0,
    SHIFT = 1,
    CONTROL = 2,
    ALT = 4,
    WIN = 8,
    CAPS_LOCK = 16,
    NUM_LOCK = 32
};

struct KeyData {
    std::string timestamp;
    uint16_t keyCode;
    uint16_t scanCode;
    uint32_t flags;
    KeyEventType eventType;
    KeyModifiers modifiers;
    std::string windowTitle;
    std::string keyName;
    
    KeyData();
    bool IsModifierKey() const;
    std::string ToString() const;
};

#endif