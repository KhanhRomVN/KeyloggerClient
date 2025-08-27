# MinGW Cross-compilation Toolchain for Windows targets
# Place this file in your project root or cmake/ directory

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Where is the target environment
set(CMAKE_FIND_ROOT_PATH 
    /usr/x86_64-w64-mingw32
    /usr/lib/gcc/x86_64-w64-mingw32
)

# Search for programs only in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers only in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Additional MinGW-specific settings
set(CMAKE_CROSSCOMPILING_EMULATOR wine64)

# Set Windows-specific flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")

# Enable static linking by default for Windows
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# Windows version targeting (Windows 7+)
add_definitions(-D_WIN32_WINNT=0x0601 -DWINVER=0x0601)