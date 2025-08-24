// File: KeyHook.h
#pragma once

#include "data/KeyData.h"
#include <functional>
#include <atomic>
#include <windows.h>

namespace hooks {

class KeyHook {
public:
    using KeyEventCallback = std::function<void(const data::KeyEvent&)>;

    KeyHook();
    ~KeyHook();

    // Delete copy constructor and assignment operator
    KeyHook(const KeyHook&) = delete;
    KeyHook& operator=(const KeyHook&) = delete;

    /**
     * @brief Installs the keyboard hook
     * @return true if hook installation successful, false otherwise
     */
    bool install();

    /**
     * @brief Removes the keyboard hook
     */
    void uninstall();

    /**
     * @brief Checks if hook is currently active
     */
    bool isActive() const;

    /**
     * @brief Sets callback for key events
     * @param callback Function to call when key event occurs
     */
    void setCallback(KeyEventCallback callback);

    /**
     * @brief Enables/disables specific key logging
     * @param enable True to enable, false to disable
     */
    void setKeyloggingEnabled(bool enable);

    /**
     * @brief Sets which keys to capture (empty for all keys)
     * @param keyCodes Vector of virtual key codes to capture
     */
    void setKeyFilter(const std::vector<int>& keyCodes);

    /**
     * @brief Emergency hook removal for security purposes
     */
    void emergencyRemove();

private:
    /**
     * @brief Static hook procedure required by Windows API
     */
    static LRESULT CALLBACK keyHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Processes key event and passes to callback
     */
    void processKeyEvent(int keyCode, KeyAction action, DWORD timestamp);

    /**
     * @brief Checks if key should be captured based on filter
     */
    bool shouldCaptureKey(int keyCode) const;

    HHOOK keyHookHandle_;
    KeyEventCallback callback_;
    std::atomic<bool> isActive_{false};
    std::atomic<bool> keyloggingEnabled_{true};
    std::vector<int> keyFilter_;
    mutable std::mutex filterMutex_;
};

} // namespace hooks