#pragma once

// Windows platform detection
#ifdef _WIN32
    #define PLATFORM_WINDOWS
    
    // Essential Windows definitions must come first
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    
    // Core Windows headers in correct order
    #include <windef.h>      // Basic Windows types (DWORD, LPSTR, etc.)
    #include <winbase.h>     // Base Windows API
    #include <winuser.h>     // User interface functions
    #include <winsvc.h>      // Service control functions
    #include <winreg.h>      // Registry functions
    #include <windows.h>     // Main Windows header
    
    // Additional Windows headers
    #include <psapi.h>       // Process API
    #include <tlhelp32.h>    // Tool Help Library
    #include <shlwapi.h>     // Shell Lightweight API
    
    // Link required libraries
    #pragma comment(lib, "user32.lib")
    #pragma comment(lib, "advapi32.lib")
    #pragma comment(lib, "psapi.lib")
    #pragma comment(lib, "shlwapi.lib")
    #pragma comment(lib, "ws2_32.lib")
    
#else
    #error "This project only supports Windows platform"
#endif

// Common includes
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>

// Platform-specific type aliases
#ifdef PLATFORM_WINDOWS
    using PlatformString = std::wstring;
    using PlatformChar = wchar_t;
    #define PLATFORM_TEXT(x) L##x
#endif
