#include "data/KeyData.h"
#include <string>
#include <windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>

// Helper function to get current timestamp
std::string GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

KeyData::KeyData()
    : timestamp(GetCurrentTimestamp()),
      keyCode(0),
      scanCode(0),
      flags(0),
      eventType(KeyEventType::KEY_DOWN),
      modifiers(KeyModifiers::NONE) {}

bool KeyData::IsModifierKey() const {
    // Sử dụng Windows virtual key codes
    return (keyCode == VK_SHIFT ||
            keyCode == VK_CONTROL ||
            keyCode == VK_MENU ||
            keyCode == VK_LWIN ||
            keyCode == VK_RWIN ||
            keyCode == VK_CAPITAL);
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