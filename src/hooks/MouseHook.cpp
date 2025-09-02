#include "hooks/MouseHook.h"
#include "data/DataManager.h"
#include "core/Logger.h"
#include "utils/TimeUtils.h"
#include "security/Obfuscation.h"
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <windows.h>

// Obfuscated strings - use char arrays instead of std::string for constexpr
const std::string OBF_MOUSEHOOK_MODULE = OBFUSCATE("MouseHook");
const std::string OBF_MOUSELOG_FORMAT = OBFUSCATE("MouseEvent: { action: %s, button: %s, pos: (%d, %d), wheel: %d }");

// Static member initialization
MouseHook* MouseHook::s_instance = nullptr;
HHOOK MouseHook::s_mouseHook = nullptr;

MouseHook::MouseHook(DataManager* dataManager)
    : m_dataManager(dataManager), m_isActive(false)
{
    s_instance = this;
}

MouseHook::~MouseHook() {
    RemoveHook();
    s_instance = nullptr;
}

bool MouseHook::InstallHook() {
    if (m_isActive) {
        return true;
    }

    s_mouseHook = SetWindowsHookExW(
        WH_MOUSE_LL,
        MouseProc,
        GetModuleHandleW(NULL),
        0
    );

    if (!s_mouseHook) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to install mouse hook. Error: " + std::to_string(error));
        return false;
    }

    m_isActive = true;
    LOG_INFO("Mouse hook installed successfully");
    return true;
}

bool MouseHook::RemoveHook() {
    if (!m_isActive) return true;

    if (s_mouseHook && !UnhookWindowsHookEx(s_mouseHook)) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to remove mouse hook. Error: " + std::to_string(error));
        return false;
    }

    s_mouseHook = nullptr;
    m_isActive = false;
    LOG_INFO("Mouse hook removed successfully");
    return true;
}

LRESULT CALLBACK MouseHook::MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= HC_ACTION && s_instance) {
        MSLLHOOKSTRUCT* mouseStruct = (MSLLHOOKSTRUCT*)lParam;
        s_instance->ProcessMouseEvent(wParam, mouseStruct);
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void MouseHook::ProcessMouseEvent(WPARAM eventType, MSLLHOOKSTRUCT* mouseStruct) {
    try {
        MouseHookData mouseData;
        mouseData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
        mouseData.position.x = mouseStruct->pt.x;
        mouseData.position.y = mouseStruct->pt.y;
        mouseData.eventType = static_cast<unsigned int>(eventType);
        mouseData.mouseData = mouseStruct->mouseData;
        mouseData.flags = mouseStruct->flags;

        // Determine which button was pressed
        mouseData.button = GetMouseButton(eventType, mouseStruct->mouseData);

        // Add wheel delta for wheel events
        if (eventType == WM_MOUSEWHEEL) {
            mouseData.wheelDelta = static_cast<short>(HIWORD(mouseStruct->mouseData));
        } else {
            mouseData.wheelDelta = 0;
        }

        if (m_dataManager) {
            m_dataManager->AddMouseData(mouseData);
        }

        // Debug logging
        if (Logger::GetLogLevel() <= LogLevel::LEVEL_DEBUG) {
            LogMouseEvent(mouseData);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Error processing mouse event: ") + e.what());
    }
}

MouseButton MouseHook::GetMouseButton(WPARAM eventType, DWORD mouseData) const {
    switch (eventType) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            return MouseButton::LEFT;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            return MouseButton::RIGHT;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            return MouseButton::MIDDLE;
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            return (HIWORD(mouseData) == XBUTTON1) ? 
                MouseButton::X1 : MouseButton::X2;
        default:
            return MouseButton::NONE;
    }
}

void MouseHook::LogMouseEvent(const MouseHookData& mouseData) {
    std::string action;
    switch (mouseData.eventType) {
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN: action = "DOWN"; break;
        case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
        case WM_XBUTTONUP: action = "UP"; break;
        case WM_MOUSEMOVE: action = "MOVE"; break;
        case WM_MOUSEWHEEL: action = "WHEEL"; break;
        default: action = "UNKNOWN"; break;
    }

    std::string button;
    switch (mouseData.button) {
        case MouseButton::LEFT: button = "LEFT"; break;
        case MouseButton::RIGHT: button = "RIGHT"; break;
        case MouseButton::MIDDLE: button = "MIDDLE"; break;
        case MouseButton::X1: button = "X1"; break;
        case MouseButton::X2: button = "X2"; break;
        default: button = "NONE"; break;
    }

    char logMessage[512];
    snprintf(logMessage, sizeof(logMessage), OBF_MOUSELOG_FORMAT.c_str(),
             action.c_str(), button.c_str(),
             mouseData.position.x, mouseData.position.y,
             mouseData.wheelDelta);

    LOG_DEBUG(logMessage);
}