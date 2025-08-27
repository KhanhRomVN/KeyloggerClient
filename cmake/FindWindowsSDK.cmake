# cmake/FindWindowsSDK.cmake - Simplified version
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(CMAKE_CROSSCOMPILING)
        # Cross-compilation mode: assume MinGW provides necessary headers
        set(WindowsSDK_FOUND TRUE)
        set(WindowsSDK_VERSION "Cross")
        set(WINDOWSSDK_INCLUDE_DIRS "")
        set(WINDOWSSDK_LIBRARIES "")
        message(STATUS "Found Windows SDK (MinGW cross-compilation)")
    else()
        # Native Windows build: try to find actual SDK
        find_path(WINDOWSSDK_INCLUDE_DIR
            NAMES windows.h
            PATHS
            "$ENV{ProgramFiles}/Windows Kits/10/Include/*/um"
            "$ENV{ProgramFiles(x86)}/Windows Kits/10/Include/*/um"
        )
        
        if(WINDOWSSDK_INCLUDE_DIR)
            set(WindowsSDK_FOUND TRUE)
            set(WINDOWSSDK_INCLUDE_DIRS ${WINDOWSSDK_INCLUDE_DIR})
            set(WINDOWSSDK_LIBRARIES "")
            message(STATUS "Found Windows SDK: ${WINDOWSSDK_INCLUDE_DIR}")
        else()
            set(WindowsSDK_FOUND FALSE)
            message(FATAL_ERROR "Windows SDK not found")
        endif()
    endif()
else()
    set(WindowsSDK_FOUND FALSE)
    message(STATUS "Windows SDK not needed for ${CMAKE_SYSTEM_NAME}")
endif()

# cmake/FindCrypto.cmake - Simplified version
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(CMAKE_CROSSCOMPILING)
        # MinGW cross-compilation
        set(Crypto_FOUND TRUE)
        set(CRYPTO_INCLUDE_DIRS "")
        set(CRYPTO_LIBRARIES advapi32 crypt32)
        message(STATUS "Found Crypto libraries (MinGW)")
    else()
        # Native Windows
        find_library(ADVAPI32_LIB advapi32)
        find_library(CRYPT32_LIB crypt32)
        
        if(ADVAPI32_LIB AND CRYPT32_LIB)
            set(Crypto_FOUND TRUE)
            set(CRYPTO_INCLUDE_DIRS "")
            set(CRYPTO_LIBRARIES ${ADVAPI32_LIB} ${CRYPT32_LIB})
            message(STATUS "Found Crypto libraries")
        else()
            set(Crypto_FOUND FALSE)
            message(FATAL_ERROR "Crypto libraries not found")
        endif()
    endif()
else()
    set(Crypto_FOUND FALSE)
    message(STATUS "Crypto libraries not needed for ${CMAKE_SYSTEM_NAME}")
endif()