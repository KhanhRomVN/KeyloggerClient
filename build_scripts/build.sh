#!/bin/bash
set -e

# ================================
# Config
# ================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
DIST_DIR="$SCRIPT_DIR/../dist" 
SRC_DIR="$SCRIPT_DIR/.."
TOOLS_DIR="$SCRIPT_DIR/../tools"

# Windows target config
MINGW_PREFIX="x86_64-w64-mingw32"
CMAKE_TOOLCHAIN_FILE="$SCRIPT_DIR/windows-cross-compile.cmake"

# Output files
INPUT_FILE="$DIST_DIR/KeyLoggerClient.exe"
OUTPUT_FILE="$DIST_DIR/KeyLoggerClient_protected.exe"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# ================================
# Check and setup CMake
# ================================
info "Checking CMake installation..."

CMAKE_LOCAL_DIR="$TOOLS_DIR/cmake-4.1.0-linux-x86_64"
CMAKE_LOCAL_BIN="$CMAKE_LOCAL_DIR/bin/cmake"

# Check if local CMake exists first
if [ -f "$CMAKE_LOCAL_BIN" ]; then
    info "Found local CMake: $CMAKE_LOCAL_BIN"
    export PATH="$CMAKE_LOCAL_DIR/bin:$PATH"
    CMAKE_CMD="$CMAKE_LOCAL_BIN"
elif command -v cmake &> /dev/null; then
    info "Using system CMake: $(which cmake)"
    CMAKE_CMD="cmake"
else
    error "CMake not found!"
    echo "Please either:"
    echo "  1. Install system CMake: sudo apt install cmake"
    echo "  2. Extract cmake-4.1.0-linux-x86_64.tar.gz to $CMAKE_LOCAL_DIR"
    exit 1
fi

# Verify CMake version
CMAKE_VERSION=$($CMAKE_CMD --version | head -n1)
info "CMake version: $CMAKE_VERSION"

# Check MinGW cross-compiler
if ! command -v ${MINGW_PREFIX}-gcc &> /dev/null; then
    error "MinGW cross-compiler not found"
    echo "Please install:"
    echo "  Ubuntu/Debian: sudo apt install mingw-w64"
    echo "  Fedora: sudo dnf install mingw64-gcc mingw64-gcc-c++"
    echo "  Arch: sudo pacman -S mingw-w64-gcc"
    exit 1
fi

# Check make
if ! command -v make &> /dev/null; then
    error "Make not found. Please install build-essential"
    exit 1
fi

success "All dependencies found"

# ================================
# Create directories
# ================================
info "Creating directories..."
mkdir -p "$BUILD_DIR"
mkdir -p "$DIST_DIR"
mkdir -p "$TOOLS_DIR"
mkdir -p "$SRC_DIR/cmake"  # Create cmake modules directory

# ================================
# Create FindWindowsSDK.cmake if it doesn't exist
# ================================
CMAKE_MODULE_FILE="$SRC_DIR/cmake/FindWindowsSDK.cmake"
if [ ! -f "$CMAKE_MODULE_FILE" ]; then
    info "Creating FindWindowsSDK.cmake module..."
    cat > "$CMAKE_MODULE_FILE" << 'EOF'
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
EOF
fi

# ================================
# Create CMake toolchain file
# ================================
info "Creating CMake toolchain file..."
cat > "$CMAKE_TOOLCHAIN_FILE" << EOF
# CMake toolchain file for cross-compiling to Windows

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross compiler
set(CMAKE_C_COMPILER ${MINGW_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${MINGW_PREFIX}-windres)

# Where to find libraries and headers
set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_PREFIX})

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set additional flags
set(CMAKE_C_FLAGS "-static-libgcc" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++" CACHE STRING "CXX flags")
set(CMAKE_EXE_LINKER_FLAGS "-static" CACHE STRING "Linker flags")

# Add module path for custom FindXXX.cmake files
set(CMAKE_MODULE_PATH "\${CMAKE_SOURCE_DIR}/cmake" \${CMAKE_MODULE_PATH})
EOF

# ================================
# Configure build
# ================================
info "Configuring build..."
cd "$BUILD_DIR"

$CMAKE_CMD -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" \
           -DCMAKE_BUILD_TYPE=Release \
           -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$DIST_DIR" \
           -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="$DIST_DIR" \
           -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="$DIST_DIR" \
           -DCMAKE_MODULE_PATH="$SRC_DIR/cmake" \
           "$SRC_DIR"

if [ $? -ne 0 ]; then
    error "CMake configuration failed"
    exit 1
fi

# ================================
# Build project
# ================================
info "Building project..."

# Get number of CPU cores for parallel build
CORES=$(nproc)
info "Using $CORES parallel jobs"

make -j$CORES

if [ $? -ne 0 ]; then
    error "Build failed"
    exit 1
fi

# ================================
# Post-build processing
# ================================
info "Post-build processing..."

# Check if main executable exists
if [ ! -f "$INPUT_FILE" ]; then
    error "Main executable not found: $INPUT_FILE"
    exit 1
fi

# Copy to final output (placeholder for obfuscation/signing)
cp "$INPUT_FILE" "$OUTPUT_FILE"

# Strip debug symbols to reduce size
info "Stripping debug symbols..."
${MINGW_PREFIX}-strip "$OUTPUT_FILE"

# ================================
# Security enhancements (optional)
# ================================
info "Applying security enhancements..."

# UPX packing (if available)
if command -v upx &> /dev/null; then
    info "Compressing with UPX..."
    upx --best "$OUTPUT_FILE" || warning "UPX compression failed"
else
    warning "UPX not found, skipping compression"
fi

# ================================
# Verify and report
# ================================
if [ -f "$OUTPUT_FILE" ]; then
    success "Build completed successfully!"
    echo
    info "Output file: $OUTPUT_FILE"
    info "File size: $(du -h "$OUTPUT_FILE" | cut -f1)"
    info "File type: $(file "$OUTPUT_FILE")"
    echo
    
    # Test if it's a valid Windows PE
    if file "$OUTPUT_FILE" | grep -q "PE32+"; then
        success "Valid Windows PE executable created"
    else
        warning "Output may not be a valid Windows executable"
    fi
else
    error "Final binary was not created"
    exit 1
fi

info "Build script completed"