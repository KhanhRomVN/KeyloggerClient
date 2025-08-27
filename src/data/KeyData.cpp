#include "data/KeyData.h"
#include "utils/TimeUtils.h"
#include <vector>
#include <cstdint>
#include <string>
#include <windows.h> // VK_SHIFT, VK_CONTROL, etc.

KeyData::KeyData()
    : timestamp(utils::TimeUtils::GetCurrentTimestamp()),
      keyCode(0),
      scanCode(0),
      flags(0),
      eventType(KeyEventType::KEY_DOWN),
      modifiers(KeyModifiers::NONE) {}

bool KeyData::IsModifierKey() const {
    return (keyCode == VK_SHIFT || keyCode == VK_CONTROL ||
            keyCode == VK_MENU || keyCode == VK_LWIN ||
            keyCode == VK_RWIN || keyCode == VK_CAPITAL);
}

std::string KeyData::ToString() const {
    std::string eventStr = (eventType == KeyEventType::KEY_DOWN) ? "DOWN" : "UP";
    std::string modStr;
    
    if (modifiers & KeyModifiers::SHIFT)   modStr += "SHIFT+";
    if (modifiers & KeyModifiers::CONTROL) modStr += "CTRL+";
    if (modifiers & KeyModifiers::ALT)     modStr += "ALT+";
    if (modifiers & KeyModifiers::WIN)     modStr += "WIN+";
    if (modStr.empty()) modStr = "NONE";
    else if (modStr.back() == '+') modStr.pop_back();

    return "KeyEvent[time=" + timestamp + ", action=" + eventStr +
           ", key=" + keyName + ", mods=" + modStr +
           ", window=" + windowTitle + "]";
}
