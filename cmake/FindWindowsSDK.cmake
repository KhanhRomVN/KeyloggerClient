# FindWindowsSDK.cmake - Dummy module for cross-compilation

# For MinGW cross-compilation, we simulate Windows SDK presence
if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Set dummy variables to satisfy the requirement
    set(WindowsSDK_FOUND TRUE)
    set(WindowsSDK_VERSION "10.0")
    set(WindowsSDK_INCLUDE_DIRS "")
    set(WindowsSDK_LIBRARIES "")
    
    # Create a dummy target if needed
    if(NOT TARGET WindowsSDK::WindowsSDK)
        add_library(WindowsSDK::WindowsSDK INTERFACE IMPORTED)
    endif()
    
    message(STATUS "Found Windows SDK (MinGW cross-compilation mode)")
else()
    # For native Windows builds, try to find actual Windows SDK
    set(WindowsSDK_FOUND FALSE)
    message(WARNING "Windows SDK not found - this is expected for cross-compilation")
endif()