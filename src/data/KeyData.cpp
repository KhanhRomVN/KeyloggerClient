#include "data/KeyData.h"
#include "utils/TimeUtils.h"
#include <vector>
#include <cstdint>
#include <string>

KeyData::KeyData()
    : timestamp(utils::TimeUtils::GetCurrentTimestamp()),
      keyCode(0),
      scanCode(0),
      flags(0),
      eventType(KeyEventType::KEY_DOWN),
      modifiers(KeyModifiers::NONE) {}

bool KeyData::IsModifierKey() const {
    // Sử dụng virtual key codes cross-platform
    return (keyCode == static_cast<int>(VirtualKey::VK_SHIFT) ||
            keyCode == static_cast<int>(VirtualKey::VK_CONTROL) ||
            keyCode == static_cast<int>(VirtualKey::VK_MENU) ||
            keyCode == static_cast<int>(VirtualKey::VK_LWIN) ||
            keyCode == static_cast<int>(VirtualKey::VK_RWIN) ||
            keyCode == static_cast<int>(VirtualKey::VK_CAPITAL));
}

std::string KeyData::ToString() const {
    std::string eventStr = (eventType == KeyEventType::KEY_DOWN) ? "DOWN" : "UP";
    std::string modStr;
    
    if (HasModifier(modifiers, KeyModifiers::SHIFT))   modStr += "SHIFT+";
    if (HasModifier(modifiers, KeyModifiers::CONTROL)) modStr += "CTRL+";
    if (HasModifier(modifiers, KeyModifiers::ALT))     modStr += "ALT+";
    if (HasModifier(modifiers, KeyModifiers::WIN))     modStr += "WIN+";
    
    if (modStr.empty()) {
        modStr = "NONE";
    } else if (modStr.back() == '+') {
        modStr.pop_back();
    }

    return "KeyEvent[time=" + timestamp + ", action=" + eventStr +
           ", key=" + keyName + ", mods=" + modStr +
           ", window=" + windowTitle + "]";
}