# FindCrypto.cmake - Dummy module for MinGW cross-compilation

if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # For MinGW cross-compilation, set dummy variables
    set(Crypto_FOUND TRUE)
    set(CRYPTO_INCLUDE_DIRS "")
    set(CRYPTO_LIBRARIES advapi32 crypt32)
    
    message(STATUS "Found Crypto (MinGW cross-compilation mode)")
else()
    # For native Windows builds, try to find actual Crypto libraries
    find_path(CRYPTO_INCLUDE_DIR wincrypt.h)
    find_library(ADVAPI32_LIBRARY advapi32)
    find_library(CRYPT32_LIBRARY crypt32)

    if(CRYPTO_INCLUDE_DIR AND ADVAPI32_LIBRARY AND CRYPT32_LIBRARY)
        set(Crypto_FOUND TRUE)
        set(CRYPTO_INCLUDE_DIRS ${CRYPTO_INCLUDE_DIR})
        set(CRYPTO_LIBRARIES ${ADVAPI32_LIBRARY} ${CRYPT32_LIBRARY})
    else()
        set(Crypto_FOUND FALSE)
    endif()
    
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Crypto DEFAULT_MSG CRYPTO_LIBRARIES CRYPTO_INCLUDE_DIRS)
endif()