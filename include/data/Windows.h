#ifndef WINDOWS_H
#define WINDOWS_H

#include <string>
#include <cstdint>
#include <vector>

struct WindowInfo {
    uint64_t handle;
    std::string title;
    std::string processName;
    std::string className;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    bool isVisible;
    bool isForeground;
    
    WindowInfo();
    std::string ToString() const;
};

class WindowManager {
public:
    static WindowInfo GetForegroundWindow();
    static WindowInfo GetWindowFromHandle(uint64_t handle);
    static std::vector<WindowInfo> GetAllWindows();
    static bool IsWindowValid(uint64_t handle);
    static std::string GetWindowTitle(uint64_t handle);
    static std::string GetProcessName(uint64_t handle);
    
private:
    WindowManager() = delete;
    ~WindowManager() = delete;
};

#endif // WINDOW_H