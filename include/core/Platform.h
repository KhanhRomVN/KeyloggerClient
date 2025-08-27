#pragma once

// ===========================================
//  平台检测 (Platform Detection)
// ===========================================
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_LINUX   0
#elif defined(__linux__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_LINUX   1
#else
    #error "Unsupported platform"
#endif

// ===========================================
//  通用类型定义 (Common Type Definitions)
// ===========================================
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>

// ===========================================
//  平台特定包含 (Platform Specific Includes)
// ===========================================
#if PLATFORM_WINDOWS
    #define NOMINMAX
    #include <windows.h>
    #include <wincrypt.h>
    #include <iphlpapi.h>
    #include <psapi.h>
    #include <tchar.h>
    #include <shlobj.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "crypt32.lib")
#elif PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <dlfcn.h>
    #include <dirent.h>
    #include <signal.h>
    #include <errno.h>
#endif

// ===========================================
//  通用宏 (Common Macros)
// ===========================================
#define UNUSED(x) (void)(x)

// 平台相关函数封装 (Cross-platform utility)
namespace Platform {

// 获取当前进程ID
inline uint32_t GetProcessId() {
#if PLATFORM_WINDOWS
    return static_cast<uint32_t>(::GetCurrentProcessId());
#elif PLATFORM_LINUX
    return static_cast<uint32_t>(::getpid());
#endif
}

// 睡眠 (毫秒)
inline void SleepMs(uint32_t ms) {
#if PLATFORM_WINDOWS
    ::Sleep(ms);
#elif PLATFORM_LINUX
    ::usleep(ms * 1000);
#endif
}

// 获取临时目录路径
inline std::string GetTempPath() {
#if PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    DWORD len = ::GetTempPathA(MAX_PATH, buffer);
    if (len == 0 || len > MAX_PATH) {
        throw std::runtime_error("Failed to get temp path");
    }
    return std::string(buffer);
#elif PLATFORM_LINUX
    const char* tmp = getenv("TMPDIR");
    if (!tmp) tmp = "/tmp";
    return std::string(tmp);
#endif
}

} // namespace Platform
