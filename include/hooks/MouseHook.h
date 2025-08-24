// File: MouseHook.h
#pragma once

#include "data/MouseData.h"
#include <functional>
#include <atomic>
#include <windows.h>

namespace hooks {

class MouseHook {
public:
    using MouseEventCallback = std::function<void(const data::MouseEvent&)>;

    MouseHook();
    ~MouseHook();

    // Delete copy constructor and assignment operator
    MouseHook(const MouseHook&) = delete;
    MouseHook& operator=(const MouseHook&) = delete;

    /**
     * @brief Installs the mouse hook
     * @return true if hook installation successful, false otherwise
     */
    bool install();

    /**
     * @brief Removes the mouse hook
     */
    void uninstall();

    /**
     * @brief Checks if hook is currently active
     */
    bool isActive() const;

    /**
     * @brief Sets callback for mouse events
     * @param callback Function to call when mouse event occurs
     */
    void setCallback(MouseEventCallback callback);

    /**
     * @brief Enables/disables specific mouse event logging
     * @param enableClicks True to enable click capture
     * @param enableMovement True to enable movement capture
     * @param enableScroll True to enable scroll capture
     */
    void setMouseCaptureEnabled(bool enableClicks, bool enableMovement, bool enableScroll);

    /**
     * @brief Sets movement capture sensitivity
     * @param pixelThreshold Only capture movement after this many pixels
     */
    void setMovementSensitivity(int pixelThreshold);

    /**
     * @brief Emergency hook removal for security purposes
     */
    void emergencyRemove();

private:
    /**
     * @brief Static hook procedure required by Windows API
     */
    static LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Processes mouse event and passes to callback
     */
    void processMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Converts Windows message to MouseAction
     */
    data::MouseAction messageToAction(UINT message) const;

    HHOOK mouseHookHandle_;
    MouseEventCallback callback_;
    std::atomic<bool> isActive_{false};
    std::atomic<bool> captureClicks_{true};
    std::atomic<bool> captureMovement_{true};
    std::atomic<bool> captureScroll_{true};
    std::atomic<int> movementSensitivity_{5};
    POINT lastMousePosition_;
    mutable std::mutex positionMutex_;
};

} // namespace hooks