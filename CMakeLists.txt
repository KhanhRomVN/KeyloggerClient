# Find Windows SDK
include(FindPackageHandleStandardArgs)

find_path(WINDOWSSDK_INCLUDE_DIR
    NAMES windows.h
    PATHS
    "$ENV{ProgramFiles}/Windows Kits/10/Include/*/um"
    "$ENV{ProgramFiles(x86)}/Windows Kits/10/Include/*/um"
    CMAKE_FIND_ROOT_PATH_BOTH
)

find_library(WINDOWSSDK_LIBRARY
    NAMES kernel32 user32 advapi32
    PATHS
    "$ENV{ProgramFiles}/Windows Kits/10/Lib/*/um/x86"
    "$ENV{ProgramFiles(x86)}/Windows Kits/10/Lib/*/um/x86"
    CMAKE_FIND_ROOT_PATH_BOTH
)

find_package_handle_standard_args(WindowsSDK
    REQUIRED_VARS WINDOWSSDK_INCLUDE_DIR WINDOWSSDK_LIBRARY
)

if(WindowsSDK_FOUND)
    set(WINDOWSSDK_INCLUDE_DIRS ${WINDOWSSDK_INCLUDE_DIR})
    set(WINDOWSSDK_LIBRARIES ${WINDOWSSDK_LIBRARY})
endif()

# Find Crypto libraries
find_path(CRYPTO_INCLUDE_DIR
    NAMES wincrypt.h
    PATHS ${WINDOWSSDK_INCLUDE_DIR}
)

find_library(CRYPTO_LIBRARY
    NAMES advapi32.lib crypt32.lib
)

find_package_handle_standard_args(Crypto
    REQUIRED_VARS CRYPTO_INCLUDE_DIR CRYPTO_LIBRARY
)

if(CRYPTO_FOUND)
    set(CRYPTO_INCLUDE_DIRS ${CRYPTO_INCLUDE_DIR})
    set(CRYPTO_LIBRARIES ${CRYPTO_LIBRARY})
endif()