#!/bin/bash

echo "=== COMPREHENSIVE BUILD FIX ==="
echo

echo "[0] Fixing Windows headers first..."
# Backup original Platform.h
if [ -f "include/core/Platform.h" ]; then
    cp "include/core/Platform.h" "include/core/Platform.h.backup"
    echo "✓ Backed up Platform.h"
fi

# Create fixed Platform.h
cat > include/core/Platform.h << 'EOF'
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
EOF

echo "✓ Created fixed Platform.h"

# Check for CMakeLists.txt and add Windows-specific settings
if [ -f "CMakeLists.txt" ]; then
    echo
    echo "Adding Windows-specific CMake settings..."
    
    # Create a patch for CMakeLists.txt
    cat > cmake_windows_fix.patch << 'EOF'
# Add after project declaration
if(WIN32)
    # Windows-specific definitions
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
    
    # Required Windows libraries
    set(WINDOWS_LIBS
        user32
        advapi32
        psapi
        shlwapi
        ws2_32
        winmm
        shell32
        ole32
        oleaut32
        uuid
        gdi32
        winspool
        comdlg32
        kernel32
    )
    
    # Link Windows libraries to target
    if(TARGET KeyloggerClientProject)
        target_link_libraries(KeyloggerClientProject ${WINDOWS_LIBS})
    endif()
endif()
EOF

    # Apply the patch by adding it to CMakeLists.txt
    if ! grep -q "WIN32_LEAN_AND_MEAN" CMakeLists.txt; then
        echo "" >> CMakeLists.txt
        echo "# Windows-specific settings" >> CMakeLists.txt
        cat cmake_windows_fix.patch >> CMakeLists.txt
        echo "✓ Added Windows settings to CMakeLists.txt"
    else
        echo "✓ Windows settings already present in CMakeLists.txt"
    fi
    
    rm cmake_windows_fix.patch
fi

# Create a test compilation to verify headers
echo
echo "Testing header compilation..."
cat > test_headers.cpp << 'EOF'
#include "include/core/Platform.h"

int main() {
    // Test basic Windows types
    DWORD test_dword = 0;
    LPSTR test_lpstr = nullptr;
    LPWSTR test_lpwstr = nullptr;
    HANDLE test_handle = nullptr;
    
    // Test service structures
    SERVICE_STATUS status = {};
    SC_HANDLE scHandle = nullptr;
    
    std::cout << "Headers compiled successfully!" << std::endl;
    return 0;
}
EOF

if g++ -I. -c test_headers.cpp -o test_headers.o 2>/dev/null; then
    echo "✓ Headers compile successfully"
    rm -f test_headers.o
else
    echo "❌ Headers still have issues, checking specific errors..."
    g++ -I. -c test_headers.cpp -o test_headers.o
fi

rm -f test_headers.cpp

# Clean previous build
echo
echo "Cleaning previous build..."
rm -rf cmake-build-debug
mkdir -p cmake-build-debug

# First, let's restore a reasonable PATH to find our tools
echo "[1] Restoring PATH to find MSYS2 tools..."
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:/mingw64/bin:/usr/bin:/bin:$PATH"

# Function to find Windows executable (with .exe extension)
find_exe() {
    local cmd=$1
    local path=$(which "$cmd" 2>/dev/null)
    
    if [ -n "$path" ]; then
        # If path doesn't end with .exe, try adding it
        if [[ "$path" != *.exe ]]; then
            if [ -f "${path}.exe" ]; then
                echo "${path}.exe"
                return 0
            fi
        fi
        
        # Check if the original path exists
        if [ -f "$path" ]; then
            echo "$path"
            return 0
        fi
    fi
    
    # Try direct search in common locations
    for dir in "/c/msys64/mingw64/bin" "/mingw64/bin" "/usr/bin"; do
        if [ -f "$dir/${cmd}.exe" ]; then
            echo "$dir/${cmd}.exe"
            return 0
        fi
    done
    
    return 1
}

# Find where our tools actually are
echo "[2] Locating tools..."
GCC_PATH=$(find_exe gcc)
GPP_PATH=$(find_exe g++)
MAKE_PATH=$(find_exe make)
CMAKE_PATH=$(find_exe cmake)

echo "Found tools at:"
echo "  GCC: ${GCC_PATH:-NOT FOUND}"
echo "  G++: ${GPP_PATH:-NOT FOUND}"
echo "  Make: ${MAKE_PATH:-NOT FOUND}"
echo "  CMake: ${CMAKE_PATH:-NOT FOUND}"

# Check if we have the minimum required tools
if [ -z "$GCC_PATH" ] || [ -z "$GPP_PATH" ] || [ -z "$CMAKE_PATH" ]; then
    echo
    echo "❌ MISSING TOOLS - Searching in common locations..."
    
    # Check common MSYS2 install locations
    for msys_root in "/c/msys64" "/d/msys64" "/msys64"; do
        if [ -d "$msys_root" ]; then
            echo "Checking MSYS2 at: $msys_root"
            
            # Update tools paths if found
            [ -z "$GCC_PATH" ] && [ -f "$msys_root/mingw64/bin/gcc.exe" ] && GCC_PATH="$msys_root/mingw64/bin/gcc.exe"
            [ -z "$GPP_PATH" ] && [ -f "$msys_root/mingw64/bin/g++.exe" ] && GPP_PATH="$msys_root/mingw64/bin/g++.exe"
            [ -z "$CMAKE_PATH" ] && [ -f "$msys_root/mingw64/bin/cmake.exe" ] && CMAKE_PATH="$msys_root/mingw64/bin/cmake.exe"
            [ -z "$MAKE_PATH" ] && [ -f "$msys_root/usr/bin/make.exe" ] && MAKE_PATH="$msys_root/usr/bin/make.exe"
            
            # Also try mingw32 if mingw64 doesn't work
            [ -z "$GCC_PATH" ] && [ -f "$msys_root/mingw32/bin/gcc.exe" ] && GCC_PATH="$msys_root/mingw32/bin/gcc.exe"
            [ -z "$GPP_PATH" ] && [ -f "$msys_root/mingw32/bin/g++.exe" ] && GPP_PATH="$msys_root/mingw32/bin/g++.exe"
            [ -z "$CMAKE_PATH" ] && [ -f "$msys_root/mingw32/bin/cmake.exe" ] && CMAKE_PATH="$msys_root/mingw32/bin/cmake.exe"
        fi
    done
    
    echo "After manual search:"
    echo "  GCC: ${GCC_PATH:-STILL NOT FOUND}"
    echo "  G++: ${GPP_PATH:-STILL NOT FOUND}" 
    echo "  CMake: ${CMAKE_PATH:-STILL NOT FOUND}"
    echo "  Make: ${MAKE_PATH:-STILL NOT FOUND}"
fi

# If we still don't have tools, exit
if [ -z "$GCC_PATH" ] || [ -z "$GPP_PATH" ] || [ -z "$CMAKE_PATH" ]; then
    echo
    echo "❌ CANNOT FIND REQUIRED TOOLS"
    echo
    echo "Please install required tools in MSYS2:"
    echo "1. Open MSYS2 terminal"
    echo "2. Run: pacman -S mingw-w64-x86_64-toolchain"
    echo "3. Run: pacman -S mingw-w64-x86_64-cmake"
    echo "4. Run: pacman -S make"
    echo
    echo "Or try 32-bit versions:"
    echo "   pacman -S mingw-w64-i686-toolchain"
    echo "   pacman -S mingw-w64-i686-cmake"
    echo
    exit 1
fi

# If no make found, try using cmake --build instead
if [ -z "$MAKE_PATH" ]; then
    echo "⚠️  Make not found, will use cmake --build instead"
    USE_CMAKE_BUILD=true
else
    USE_CMAKE_BUILD=false
fi

echo
echo "[3] Testing tools work..."

echo "Testing GCC..."
if "$GCC_PATH" --version > /dev/null 2>&1; then
    echo "✓ GCC works"
    "$GCC_PATH" --version | head -1
else
    echo "✗ GCC failed"
fi

echo "Testing G++..."
if "$GPP_PATH" --version > /dev/null 2>&1; then
    echo "✓ G++ works"
    "$GPP_PATH" --version | head -1
else
    echo "✗ G++ failed"
fi

echo "Testing CMake..."
if "$CMAKE_PATH" --version > /dev/null 2>&1; then
    echo "✓ CMake works"
    "$CMAKE_PATH" --version | head -1
else
    echo "✗ CMake failed"
fi

# Quick compiler test
echo
echo "[4] Testing compiler functionality..."
echo 'int main(){return 0;}' > test_compile.cpp
if "$GPP_PATH" test_compile.cpp -o test_compile.exe 2>/dev/null; then
    echo "✓ Compiler can build executables"
    rm -f test_compile.exe
else
    echo "✗ Compiler test failed"
    echo "Testing with verbose output..."
    "$GPP_PATH" -v test_compile.cpp -o test_compile.exe
fi
rm -f test_compile.cpp

echo
echo "[5] Clean build attempt..."

# Clean previous attempts
rm -rf cmake-build-debug
mkdir -p cmake-build-debug
cd cmake-build-debug

# Convert paths to CMake format (forward slashes)
CMAKE_GCC_PATH=$(echo "$GCC_PATH" | sed 's|\\|/|g')
CMAKE_GPP_PATH=$(echo "$GPP_PATH" | sed 's|\\|/|g')

echo "Configuring with found tools:"
echo "  C Compiler: $CMAKE_GCC_PATH"
echo "  CXX Compiler: $CMAKE_GPP_PATH"
echo "  CMake: $CMAKE_PATH"
echo

# Try different generators in order of preference
generators=("MSYS Makefiles" "MinGW Makefiles" "Unix Makefiles")
cmake_success=false

for gen in "${generators[@]}"; do
    echo "Trying generator: $gen"
    
    "$CMAKE_PATH" .. \
        -G "$gen" \
        -DCMAKE_C_COMPILER="$CMAKE_GCC_PATH" \
        -DCMAKE_CXX_COMPILER="$CMAKE_GPP_PATH" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DWIN32_LEAN_AND_MEAN=ON \
        -DNOMINMAX=ON \
        2>&1 | tee cmake_config.log
    
    if [ ${PIPESTATUS[0]} -eq 0 ]; then
        echo "✅ CMAKE SUCCESS with $gen!"
        cmake_success=true
        break
    else
        echo "❌ Failed with $gen"
        echo "--- Error details ---"
        tail -10 cmake_config.log
        echo "--- End error details ---"
        echo
        
        # Clean up for next attempt
        rm -f CMakeCache.txt
        rm -rf CMakeFiles
    fi
done

if [ "$cmake_success" = true ]; then
    echo
    echo "[6] Building..."
    
    # Build the project
    if [ "$USE_CMAKE_BUILD" = true ]; then
        echo "Using cmake --build..."
        "$CMAKE_PATH" --build . --config Debug --parallel
        build_result=$?
    else
        echo "Using make..."
        "$MAKE_PATH" -j$(nproc 2>/dev/null || echo 2)
        build_result=$?
    fi
    
    if [ $build_result -eq 0 ]; then
        echo
        echo "🎉 BUILD COMPLETE!"
        
        # Find executable
        exe_found=false
        for exe_name in "KeyloggerClientProject.exe" "KeyloggerClient.exe" "main.exe"; do
            for search_dir in "." "bin" "Debug" "Release"; do
                if [ -f "$search_dir/$exe_name" ]; then
                    echo "✅ Executable: $(pwd)/$search_dir/$exe_name"
                    ls -lh "$search_dir/$exe_name"
                    exe_found=true
                    break 2
                fi
            done
        done
        
        if [ "$exe_found" = false ]; then
            echo "🔍 Searching for any executable..."
            find . -name "*.exe" -type f | head -5
        fi
        
    else
        echo "❌ Build failed"
        echo
        echo "Trying with verbose output..."
        if [ "$USE_CMAKE_BUILD" = true ]; then
            "$CMAKE_PATH" --build . --config Debug --parallel --verbose
        else
            "$MAKE_PATH" VERBOSE=1
        fi
    fi
else
    echo "❌ All CMake generators failed"
    
    echo
    echo "Available CMake generators:"
    "$CMAKE_PATH" --help | grep -A 15 "Generators" || echo "Cannot show generators"
    
    echo
    echo "System information:"
    uname -a
    echo "MSYSTEM: ${MSYSTEM:-not set}"
    echo "PATH: $PATH"
fi

cd ..

echo
echo "=== FINAL STATUS ==="
final_exe=""
for search_pattern in "cmake-build-debug/*.exe" "cmake-build-debug/bin/*.exe" "cmake-build-debug/Debug/*.exe"; do
    exe_file=$(ls $search_pattern 2>/dev/null | head -1)
    if [ -n "$exe_file" ]; then
        final_exe="$exe_file"
        break
    fi
done

if [ -n "$final_exe" ]; then
    echo "🟢 SUCCESS: Project built successfully!"
    echo "📁 Executable: $final_exe"
    echo "📊 Size: $(ls -lh "$final_exe" | awk '{print $5}')"
else
    echo "🔴 FAILED: No executable found"
    
    echo
    echo "Build directory contents:"
    ls -la cmake-build-debug/ 2>/dev/null || echo "No build directory"
    
    echo
    echo "Troubleshooting suggestions:"
    echo "1. Check if all required packages are installed"
    echo "2. Try running in different MSYS2 terminal (mingw64, mingw32, or msys)"
    echo "3. Check your CMakeLists.txt for Windows-specific issues"
    echo "4. Headers have been fixed - Windows API types should now work"
fi