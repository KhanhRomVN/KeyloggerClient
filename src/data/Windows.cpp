#include "Windows.h"
#include "core/Logger.h"
#include "core/Platform.h"
#include "utils/StringUtils.h"
#include <vector>
    
#if PLATFORM_WINDOWS
#include <Windows.h>
#include <psapi.h>
#elif PLATFORM_LINUX
#include <X11/Xlib.h>
#endif

WindowInfo::WindowInfo() 
    : handle(0), x(0), y(0), width(0), height(0), 
      isVisible(false), isForeground(false) {}

std::string WindowInfo::ToString() const {
    return "Window[handle=" + std::to_string(handle) +
           ", title='" + title + 
           "', process='" + processName +
           "', class='" + className +
           "', pos=(" + std::to_string(x) + "," + std::to_string(y) +
           "), size=(" + std::to_string(width) + "x" + std::to_string(height) +
           "), visible=" + (isVisible ? "true" : "false") +
           ", foreground=" + (isForeground ? "true" : "false") + "]";
}

#if PLATFORM_WINDOWS

WindowInfo WindowManager::GetForegroundWindow() {
    HWND hwnd = ::GetForegroundWindow();
    return GetWindowFromHandle(reinterpret_cast<uint64_t>(hwnd));
}

WindowInfo WindowManager::GetWindowFromHandle(uint64_t handle) {
    WindowInfo info;
    HWND hwnd = reinterpret_cast<HWND>(handle);
    
    if (!IsWindowValid(handle)) {
        return info;
    }
    
    info.handle = handle;
    
    // Get window title
    wchar_t title[256];
    if (GetWindowTextW(hwnd, title, sizeof(title)/sizeof(wchar_t))) {
        info.title = utils::StringUtils::WideToUtf8(title);
    } else {
        info.title = "Unknown";
    }
    
    // Get window class
    wchar_t className[256];
    if (GetClassNameW(hwnd, className, sizeof(className)/sizeof(wchar_t))) {
        info.className = utils::StringUtils::WideToUtf8(className);
    } else {
        info.className = "Unknown";
    }
    
    // Get window position and size
    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        info.x = rect.left;
        info.y = rect.top;
        info.width = rect.right - rect.left;
        info.height = rect.bottom - rect.top;
    }
    
    // Get visibility
    info.isVisible = IsWindowVisible(hwnd) != FALSE;
    
    // Get process name
    info.processName = GetProcessName(handle);
    
    // Check if foreground
    info.isForeground = (hwnd == GetForegroundWindow());
    
    return info;
}

std::vector<WindowInfo> WindowManager::GetAllWindows() {
    std::vector<WindowInfo> windows;
    
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto windows = reinterpret_cast<std::vector<WindowInfo>*>(lParam);
        if (IsWindowVisible(hwnd)) {
            windows->push_back(GetWindowFromHandle(reinterpret_cast<uint64_t>(hwnd)));
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&windows));
    
    return windows;
}

bool WindowManager::IsWindowValid(uint64_t handle) {
    HWND hwnd = reinterpret_cast<HWND>(handle);
    return IsWindow(hwnd) != FALSE;
}

std::string WindowManager::GetWindowTitle(uint64_t handle) {
    HWND hwnd = reinterpret_cast<HWND>(handle);
    wchar_t title[256];
    if (GetWindowTextW(hwnd, title, sizeof(title)/sizeof(wchar_t))) {
        return utils::StringUtils::WideToUtf8(title);
    }
    return "Unknown";
}

std::string WindowManager::GetProcessName(uint64_t handle) {
    HWND hwnd = reinterpret_cast<HWND>(handle);
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (hProcess) {
        wchar_t processName[MAX_PATH];
        DWORD size = sizeof(processName)/sizeof(wchar_t);
        if (QueryFullProcessImageNameW(hProcess, 0, processName, &size)) {
            CloseHandle(hProcess);
            std::wstring wideName(processName);
            std::string fileName = utils::StringUtils::WideToUtf8(
                wideName.substr(wideName.find_last_of(L"\\") + 1));
            return fileName;
        }
        CloseHandle(hProcess);
    }
    return "Unknown";
}

#elif PLATFORM_LINUX

WindowInfo WindowManager::GetForegroundWindow() {
    // Linux implementation would use X11
    WindowInfo info;
    // Placeholder implementation
    return info;
}

WindowInfo WindowManager::GetWindowFromHandle(uint64_t handle) {
    WindowInfo info;
    // Placeholder implementation
    return info;
}

std::vector<WindowInfo> WindowManager::GetAllWindows() {
    std::vector<WindowInfo> windows;
    // Placeholder implementation
    return windows;
}

bool WindowManager::IsWindowValid(uint64_t handle) {
    // Placeholder implementation
    return false;
}

std::string WindowManager::GetWindowTitle(uint64_t handle) {
    return "Unknown";
}

std::string WindowManager::GetProcessName(uint64_t handle) {
    return "Unknown";
}

#endif