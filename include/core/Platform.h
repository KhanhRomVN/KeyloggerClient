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
#include <memory>
#include <stdexcept>
#include <iostream>

// ===========================================
//  平台特定包含 (Platform Specific Includes)
// ===========================================
#if PLATFORM_WINDOWS
    #include <windows.h>
    #include <winsvc.h>
    #include <wincrypt.h>
    #include <iphlpapi.h>
    #include <psapi.h>
    #include <tchar.h>
    #include <shlobj.h>
    #include <windns.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "crypt32.lib")
    #pragma comment(lib, "dnsapi.lib")
#elif PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <cerrno>
    #include <sys/file.h>
    #include <cstring>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <resolv.h>
#endif

// ===========================================
//  通用宏 (Common Macros)
// ===========================================
#define UNUSED(x) (void)(x)

// ===========================================
//  Cross-platform types and handles
// ===========================================
#if PLATFORM_WINDOWS
    typedef HANDLE PlatformHandle;
    typedef DWORD PlatformError;
    #define INVALID_PLATFORM_HANDLE INVALID_HANDLE_VALUE
    #define PLATFORM_ERROR_ALREADY_EXISTS ERROR_ALREADY_EXISTS
#elif PLATFORM_LINUX
    typedef int PlatformHandle;
    typedef int PlatformError;
    #define INVALID_PLATFORM_HANDLE (-1)
    #define PLATFORM_ERROR_ALREADY_EXISTS EEXIST
#endif

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
    if (!tmp) tmp = getenv("TEMP");
    if (!tmp) tmp = "/tmp";
    return std::string(tmp);
#endif
}

// 创建命名互斥锁 (Named Mutex)
inline PlatformHandle CreateNamedMutex(const char* name) {
#if PLATFORM_WINDOWS
    return ::CreateMutexA(nullptr, TRUE, name);
#elif PLATFORM_LINUX
    std::string lockPath = GetTempPath() + "/" + name + ".lock";
    int fd = open(lockPath.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        return INVALID_PLATFORM_HANDLE;
    }
    
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        if (errno == EWOULDBLOCK) {
            close(fd);
            errno = EEXIST;
            return INVALID_PLATFORM_HANDLE;
        }
        close(fd);
        return INVALID_PLATFORM_HANDLE;
    }
    
    return fd;
#endif
}

// 获取最后错误
inline PlatformError GetLastError() {
#if PLATFORM_WINDOWS
    return ::GetLastError();
#elif PLATFORM_LINUX
    return errno;
#endif
}

// 退出进程
inline void ExitProcess(int exitCode) {
#if PLATFORM_WINDOWS
    ::ExitProcess(static_cast<UINT>(exitCode)); 
#elif PLATFORM_LINUX    
    ::exit(exitCode);   
#endif
}   

// 关闭句柄
inline bool CloseHandle(PlatformHandle handle) {
#if PLATFORM_WINDOWS
    return ::CloseHandle(handle) != FALSE;
#elif PLATFORM_LINUX
    if (handle != INVALID_PLATFORM_HANDLE) {
        close(handle);
        return true;
    }
    return false;
#endif
}

} // namespace Platform