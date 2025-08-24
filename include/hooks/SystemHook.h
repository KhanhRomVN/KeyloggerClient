// File: SystemHook.h
#pragma once

#include "data/SystemData.h"
#include <functional>
#include <atomic>
#include <windows.h>

namespace hooks {

class SystemHook {
public:
    using SystemEventCallback = std::function<void(const data::SystemEvent&)>;
    using ForegroundWindowCallback = std::function<void(HWND, const std::string&)>;

    SystemHook();
    ~SystemHook();

    // Delete copy constructor and assignment operator
    SystemHook(const SystemHook&) = delete;
    SystemHook& operator=(const SystemHook&) = delete;

    /**
     * @brief Installs system event hooks
     * @return true if hook installation successful, false otherwise
     */
    bool install();

    /**
     * @brief Removes all system hooks
     */
    void uninstall();

    /**
     * @brief Checks if hooks are currently active
     */
    bool isActive() const;

    /**
     * @brief Sets callback for system events
     * @param callback Function to call when system event occurs
     */
    void setSystemEventCallback(SystemEventCallback callback);

    /**
     * @brief Sets callback for foreground window changes
     * @param callback Function to call when foreground window changes
     */
    void setForegroundWindowCallback(ForegroundWindowCallback callback);

    /**
     * @brief Enables/disables specific system event logging
     * @param enableWindowEvents True to enable window event capture
     * @param enablePowerEvents True to enable power event capture
     * @param enableSessionEvents True to enable session event capture
     */
    void setEventCaptureEnabled(bool enableWindowEvents, bool enablePowerEvents, bool enableSessionEvents);

    /**
     * @brief Captures current system information
     * @return SystemInfo structure containing system details
     */
    data::SystemInfo captureSystemInfo() const;

    /**
     * @brief Emergency hook removal for security purposes
     */
    void emergencyRemove();

private:
    /**
     * @brief Window procedure hook for system events
     */
    static LRESULT CALLBACK systemEventProc(int nCode, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Handles WinEvent hook events
     */
    static void CALLBACK winEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, 
                                     LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

    /**
     * @brief Processes system event and passes to callback
     */
    void processSystemEvent(UINT message, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Processes window event from WinEvent hook
     */
    void processWindowEvent(DWORD event, HWND hwnd);

    /**
     * @brief Gets window title and process name from HWND
     */
    std::string getWindowInfo(HWND hwnd) const;

    HHOOK systemHookHandle_;
    HWINEVENTHOOK winEventHook_;
    SystemEventCallback systemEventCallback_;
    ForegroundWindowCallback foregroundWindowCallback_;
    std::atomic<bool> isActive_{false};
    std::atomic<bool> captureWindowEvents_{true};
    std::atomic<bool> capturePowerEvents_{true};
    std::atomic<bool> captureSessionEvents_{true};
    mutable std::mutex callbackMutex_;
};

} // namespace hooks