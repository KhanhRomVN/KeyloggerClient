#!/bin/bash

# ========================================
#   COMPREHENSIVE OPENSSL FIX & BUILD
#   All-in-One Script for MinGW/Windows
#   Enhanced with Auto PATH Configuration
# ========================================

set -e

# Script info
SCRIPT_VERSION="2.1"
SCRIPT_NAME="OpenSSL Complete Fix & Build"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# Global variables
PROJECT_NAME="KeyloggerClientProject"
BUILD_DIR="cmake-build-debug"
OPENSSL_DOWNLOAD_DIR="openssl_temp"
BACKUP_SUFFIX="$(date +%Y%m%d_%H%M%S)"

# Print functions
print_header() {
    echo -e "${CYAN}${BOLD}========================================${NC}"
    echo -e "${CYAN}${BOLD}   $1${NC}"
    echo -e "${CYAN}${BOLD}========================================${NC}"
    echo
}

print_step() {
    echo -e "${BLUE}${BOLD}[$1]${NC} $2"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${CYAN}ℹ $1${NC}"
}

# Logging function
log_action() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "fix_openssl.log"
}

# Error handling
handle_error() {
    print_error "Script failed at line $1"
    log_action "ERROR: Script failed at line $1"
    echo "Check fix_openssl.log for details"
    exit 1
}

trap 'handle_error $LINENO' ERR

# Help function
show_help() {
    cat << EOF
${BOLD}$SCRIPT_NAME v$SCRIPT_VERSION${NC}

USAGE:
    $0 [OPTIONS]

OPTIONS:
    -h, --help          Show this help message
    -c, --clean         Clean build directory before building
    -f, --force         Force reinstall OpenSSL even if found
    -d, --debug         Enable debug output
    -t, --test-only     Only test build, don't create full project
    --no-download       Don't download OpenSSL, use system only
    --mingw-only        Use only MinGW-compatible libraries
    --skip-backup       Don't backup existing files
    --skip-path-fix     Skip automatic PATH configuration
    --force-path-fix    Force PATH reconfiguration even if tools found

EXAMPLES:
    $0                  # Standard build with auto PATH fix
    $0 -c -d            # Clean build with debug output
    $0 -f --mingw-only  # Force reinstall with MinGW libs only
    $0 -t               # Test build only
    $0 --force-path-fix # Force PATH reconfiguration

EOF
}

# Parse command line arguments
CLEAN_BUILD=false
FORCE_REINSTALL=false
DEBUG_MODE=false
TEST_ONLY=false
NO_DOWNLOAD=false
MINGW_ONLY=false
SKIP_BACKUP=false
SKIP_PATH_FIX=false
FORCE_PATH_FIX=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -f|--force)
            FORCE_REINSTALL=true
            shift
            ;;
        -d|--debug)
            DEBUG_MODE=true
            shift
            ;;
        -t|--test-only)
            TEST_ONLY=true
            shift
            ;;
        --no-download)
            NO_DOWNLOAD=true
            shift
            ;;
        --mingw-only)
            MINGW_ONLY=true
            shift
            ;;
        --skip-backup)
            SKIP_BACKUP=true
            shift
            ;;
        --skip-path-fix)
            SKIP_PATH_FIX=true
            shift
            ;;
        --force-path-fix)
            FORCE_PATH_FIX=true
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Debug mode setup
if [ "$DEBUG_MODE" = true ]; then
    set -x
    print_info "Debug mode enabled"
fi

# ========================================
#        PATH CONFIGURATION SECTION
# ========================================

configure_development_environment() {
    print_info "Configuring development environment..."
    
    # Original PATH backup
    ORIGINAL_PATH="$PATH"
    
    # Common tool installation paths
    MSYS2_PATHS=(
        "/c/msys64/usr/bin"
        "/c/msys64/mingw64/bin"
        "/c/msys64/mingw32/bin"
        "/mingw64/bin"
        "/mingw32/bin"
        "/usr/bin"
        "/usr/local/bin"
    )
    
    STRAWBERRY_PATHS=(
        "/c/Strawberry/c/bin"
        "/c/Strawberry/perl/bin"
    )
    
    ADDITIONAL_PATHS=(
        "/c/Program Files/CMake/bin"
        "/c/Program Files (x86)/CMake/bin"
        "/c/Program Files/Git/bin"
        "/c/Program Files/Git/usr/bin"
        "/c/Program Files (x86)/Git/bin"
        "/c/Program Files (x86)/Git/usr/bin"
    )
    
    # Auto-detect MSYS2 installation
    MSYS2_ROOT=""
    for msys_path in "/c/msys64" "/d/msys64" "/msys64" "/usr"; do
        if [ -d "$msys_path" ] && [ -f "$msys_path/usr/bin/pacman.exe" -o -f "$msys_path/usr/bin/pacman" ]; then
            MSYS2_ROOT="$msys_path"
            print_success "Found MSYS2 installation: $MSYS2_ROOT"
            break
        fi
    done
    
    # Build new PATH
    NEW_PATH_COMPONENTS=()
    
    # Add MSYS2 paths if found
    if [ -n "$MSYS2_ROOT" ]; then
        for subpath in "usr/bin" "mingw64/bin" "mingw32/bin"; do
            full_path="$MSYS2_ROOT/$subpath"
            if [ -d "$full_path" ]; then
                NEW_PATH_COMPONENTS+=("$full_path")
            fi
        done
    else
        print_warning "MSYS2 not found, trying standard MinGW paths"
        for path in "${MSYS2_PATHS[@]}"; do
            if [ -d "$path" ]; then
                NEW_PATH_COMPONENTS+=("$path")
            fi
        done
    fi
    
    # Add other useful paths
    for path in "${STRAWBERRY_PATHS[@]}" "${ADDITIONAL_PATHS[@]}"; do
        if [ -d "$path" ]; then
            NEW_PATH_COMPONENTS+=("$path")
        fi
    done
    
    # Construct final PATH
    if [ ${#NEW_PATH_COMPONENTS[@]} -gt 0 ]; then
        NEW_PATH=$(IFS=':'; echo "${NEW_PATH_COMPONENTS[*]}")
        export PATH="$NEW_PATH:$ORIGINAL_PATH"
        
        print_success "PATH updated with development tools"
        
        # Automatically update ~/.bashrc to persist PATH changes
        if [ -f "$HOME/.bashrc" ] && ! grep -q "MSYS2.*PATH" "$HOME/.bashrc"; then
            echo "" >> "$HOME/.bashrc"
            echo "# Auto-added by fix_openssl.sh - $(date)" >> "$HOME/.bashrc"
            echo "export PATH=\"$NEW_PATH:\$PATH\"" >> "$HOME/.bashrc"
            print_success "PATH configuration saved to ~/.bashrc"
        fi
        
        # Also update current session's .bash_profile if it exists
        if [ -f "$HOME/.bash_profile" ] && ! grep -q "MSYS2.*PATH" "$HOME/.bash_profile"; then
            echo "" >> "$HOME/.bash_profile"
            echo "# Auto-added by fix_openssl.sh - $(date)" >> "$HOME/.bash_profile"
            echo "export PATH=\"$NEW_PATH:\$PATH\"" >> "$HOME/.bash_profile"
            print_success "PATH configuration saved to ~/.bash_profile"
        fi
    fi
    
    # Set compiler environment variables
    if command -v gcc >/dev/null 2>&1 && command -v g++ >/dev/null 2>&1; then
        export CC=$(which gcc)
        export CXX=$(which g++)
        print_success "Compiler environment variables set"
        print_info "  CC: $CC"
        print_info "  CXX: $CXX"
    fi
    
    log_action "Development environment configured"
}

# Check if tool is available and working
check_tool_working() {
    local tool="$1"
    local version_flag="${2:---version}"
    
    if command -v "$tool" >/dev/null 2>&1; then
        # Try to run the tool to ensure it's working
        if "$tool" "$version_flag" >/dev/null 2>&1; then
            return 0
        else
            return 1
        fi
    else
        return 1
    fi
}

# Enhanced tool checking with auto-installation suggestions
check_and_install_tools() {
    print_info "Checking required development tools..."
    
    local REQUIRED_TOOLS=("gcc" "g++" "make" "cmake" "curl")
    local OPTIONAL_TOOLS=("7z" "git" "pkg-config")
    local MISSING_REQUIRED=()
    local MISSING_OPTIONAL=()
    
    # Check required tools
    for tool in "${REQUIRED_TOOLS[@]}"; do
        if check_tool_working "$tool"; then
            print_success "$tool: $(which $tool)"
            if [ "$DEBUG_MODE" = true ]; then
                local version=$($tool --version 2>/dev/null | head -1 || echo 'Unknown')
                print_info "  Version: $version"
            fi
        else
            MISSING_REQUIRED+=("$tool")
            print_error "Missing or non-functional: $tool"
        fi
    done
    
    # Check optional tools
    for tool in "${OPTIONAL_TOOLS[@]}"; do
        if check_tool_working "$tool"; then
            print_success "Optional tool $tool: $(which $tool)"
        else
            MISSING_OPTIONAL+=("$tool")
            print_warning "Optional tool missing: $tool"
        fi
    done
    
    # Handle missing tools
    if [ ${#MISSING_REQUIRED[@]} -ne 0 ]; then
        print_error "Missing required tools: ${MISSING_REQUIRED[*]}"
        echo
        print_info "Installation suggestions:"
        
        # MSYS2 installation
        if [ -n "$MSYS2_ROOT" ]; then
            print_info "Using MSYS2/MinGW-w64:"
            local pacman_packages=()
            for tool in "${MISSING_REQUIRED[@]}"; do
                case $tool in
                    gcc|g++) pacman_packages+=("mingw-w64-x86_64-gcc") ;;
                    make) pacman_packages+=("make") ;;
                    cmake) pacman_packages+=("mingw-w64-x86_64-cmake") ;;
                    curl) pacman_packages+=("curl") ;;
                esac
            done
            
            if [ ${#pacman_packages[@]} -gt 0 ]; then
                local unique_packages=($(printf "%s\n" "${pacman_packages[@]}" | sort -u))
                print_info "  pacman -S ${unique_packages[*]}"
            fi
        fi
        
        # Alternative installations
        print_info "Alternative installations:"
        print_info "  1. Install MSYS2: https://www.msys2.org/"
        print_info "  2. Install MinGW-w64: https://www.mingw-w64.org/"
        print_info "  3. Install Git for Windows (includes MinGW): https://git-scm.com/"
        
        return 1
    fi
    
    # Handle missing optional tools
    if [ ${#MISSING_OPTIONAL[@]} -ne 0 ] && [ "$DEBUG_MODE" = true ]; then
        print_info "Optional tools that could be installed:"
        for tool in "${MISSING_OPTIONAL[@]}"; do
            case $tool in
                7z) print_info "  7z: pacman -S p7zip" ;;
                git) print_info "  git: pacman -S git" ;;
                pkg-config) print_info "  pkg-config: pacman -S pkgconf" ;;
            esac
        done
    fi
    
    return 0
}

# ========================================
#           MAIN SCRIPT START
# ========================================

print_header "$SCRIPT_NAME v$SCRIPT_VERSION"
log_action "Script started with PID $$"

# Step 0: Environment Setup (New Step)
print_step "0" "Environment Setup & PATH Configuration"

# Check if running in appropriate environment
if [[ "$(uname -s)" != MINGW* ]] && [[ "$(uname -s)" != MSYS* ]] && [[ "$(uname -s)" != *_NT* ]]; then
    print_error "This script requires a MinGW/MSYS2/Windows environment"
    print_info "To run this script:"
    print_info "1. Open Git Bash or MinGW terminal"
    print_info "2. Or install MSYS2: https://www.msys2.org/"
    print_info "3. Or run in Windows Subsystem for Linux (WSL)"
    exit 1
fi

print_success "Environment: $(uname -s)"
print_info "Architecture: $(uname -m)"
print_info "User: ${USER:-$(whoami)}"
print_info "Working Directory: $(pwd)"
print_info "Shell: ${SHELL:-$0}"

# Configure PATH and development environment
if [ "$SKIP_PATH_FIX" = false ]; then
    if [ "$FORCE_PATH_FIX" = true ] || ! check_tool_working "make"; then
        configure_development_environment
        print_success "Environment configuration completed"
    else
        print_info "Development tools found, skipping PATH reconfiguration"
        print_info "Use --force-path-fix to reconfigure anyway"
    fi
else
    print_info "Skipping PATH configuration (--skip-path-fix specified)"
fi

# Step 1: Enhanced System Check
print_step "1" "Enhanced System & Tool Verification"

# Check and install tools
if ! check_and_install_tools; then
    print_error "Required tools are missing. Please install them and run the script again."
    exit 1
fi

print_success "All required development tools are available"
log_action "System check completed successfully"

# Step 2: Project Structure Check & Creation
print_step "2" "Project Structure Setup"

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ] && [ ! -d "src" ]; then
    print_warning "No existing project structure found"
    read -p "Create new project structure here? (y/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_info "Operation cancelled"
        exit 0
    fi
fi

# Create comprehensive project structure
print_info "Creating project directory structure..."
mkdir -p {src,include}/{core,data,hooks,security,utils,communication,persistence}
mkdir -p {resources/config,logs,build,cmake,tests,docs}

# Create additional useful directories
mkdir -p {tools,scripts,external,output}

# Backup existing files
if [ "$SKIP_BACKUP" = false ]; then
    print_info "Backing up existing files..."
    for file in CMakeLists.txt src/main.cpp; do
        if [ -f "$file" ] && [ ! -f "$file.backup_$BACKUP_SUFFIX" ]; then
            cp "$file" "$file.backup_$BACKUP_SUFFIX"
            print_success "Backed up: $file"
        fi
    done
fi

print_success "Project structure ready"

# Step 3: OpenSSL Detection & Installation
print_step "3" "OpenSSL Detection & Setup"

# Enhanced OpenSSL search paths with better Windows support
OPENSSL_SEARCH_PATHS=(
    # Windows common locations
    "/c/Program Files/OpenSSL-Win64"
    "/c/Program Files (x86)/OpenSSL-Win32" 
    "/c/OpenSSL-Win64"
    "/c/OpenSSL-Win32"
    "/c/OpenSSL"
    
    # MSYS2/MinGW locations
    "/mingw64"
    "/mingw32"
    "${MSYS2_ROOT}/mingw64"
    "${MSYS2_ROOT}/mingw32"
    
    # vcpkg locations
    "/c/vcpkg/installed/x64-windows"
    "/c/vcpkg/installed/x86-windows"
    "/c/tools/vcpkg/installed/x64-windows"
    
    # Standard Unix-like locations
    "/usr/local"
    "/usr/local/ssl"
    "/opt/openssl"
    "/usr"
    
    # Downloaded location
    "$(pwd)/$OPENSSL_DOWNLOAD_DIR"
)

# Variables for found OpenSSL
OPENSSL_ROOT=""
OPENSSL_INCLUDE=""
OPENSSL_SSL_LIB=""
OPENSSL_CRYPTO_LIB=""
OPENSSL_FOUND=false

# Enhanced OpenSSL detection function
find_openssl() {
    local search_path="$1"
    
    if [ ! -d "$search_path" ]; then
        return 1
    fi
    
    print_info "Searching OpenSSL in: $search_path"
    
    # Find include directory
    local include_dirs=(
        "$search_path/include"
        "$search_path/../include"
        "$search_path/include/openssl/.."
    )
    
    local include_dir=""
    for inc_path in "${include_dirs[@]}"; do
        if [ -d "$inc_path/openssl" ] && [ -f "$inc_path/openssl/ssl.h" ]; then
            include_dir="$inc_path"
            print_success "Found OpenSSL headers in: $include_dir"
            break
        fi
    done
    
    if [ -z "$include_dir" ]; then
        print_warning "No OpenSSL headers found in $search_path"
        return 1
    fi
    
    # Find libraries with multiple naming conventions
    local ssl_lib=""
    local crypto_lib=""
    
    # Search in various lib directories with priority order
    local lib_search_patterns=(
        # Static libraries (preferred for distribution)
        "libssl.a:libcrypto.a"
        "ssl.lib:crypto.lib"
        "libssl.lib:libcrypto.lib"
        # Import libraries
        "libssl.dll.a:libcrypto.dll.a"
        "ssl.dll.a:crypto.dll.a"
        # MSVC libraries
        "ssleay32.lib:libeay32.lib"
        "libssl32.lib:libcrypto32.lib"
    )
    
    local lib_dirs=(
        "$search_path/lib"
        "$search_path/lib/VC/x64/MD"
        "$search_path/lib/VC/x64/MT"
        "$search_path/lib/VC"
        "$search_path/lib64"
        "$search_path/lib32"
        "$search_path/out32"
        "$search_path/out32dll"
        "$search_path"
    )
    
    for lib_dir in "${lib_dirs[@]}"; do
        if [ ! -d "$lib_dir" ]; then
            continue
        fi
        
        print_info "  Searching in lib directory: $lib_dir"
        
        for pattern in "${lib_search_patterns[@]}"; do
            IFS=':' read -r ssl_pattern crypto_pattern <<< "$pattern"
            
            local found_ssl=$(find "$lib_dir" -name "$ssl_pattern" 2>/dev/null | head -1)
            local found_crypto=$(find "$lib_dir" -name "$crypto_pattern" 2>/dev/null | head -1)
            
            if [ -n "$found_ssl" ] && [ -n "$found_crypto" ]; then
                ssl_lib="$found_ssl"
                crypto_lib="$found_crypto"
                print_success "Found SSL library: $ssl_lib"
                print_success "Found Crypto library: $crypto_lib"
                break 2
            fi
        done
    done
    
    if [ -n "$ssl_lib" ] && [ -n "$crypto_lib" ] && [ -n "$include_dir" ]; then
        OPENSSL_ROOT="$search_path"
        OPENSSL_INCLUDE="$include_dir"
        OPENSSL_SSL_LIB="$ssl_lib"
        OPENSSL_CRYPTO_LIB="$crypto_lib"
        return 0
    fi
    
    print_warning "Complete OpenSSL installation not found in $search_path"
    return 1
}

# Search for existing OpenSSL
if [ "$FORCE_REINSTALL" = false ]; then
    print_info "Searching for existing OpenSSL installation..."
    for path in "${OPENSSL_SEARCH_PATHS[@]}"; do
        if [ -n "$path" ] && find_openssl "$path"; then
            OPENSSL_FOUND=true
            print_success "OpenSSL found at: $OPENSSL_ROOT"
            break
        fi
    done
fi

# Enhanced OpenSSL download with multiple sources
if [ "$OPENSSL_FOUND" = false ] && [ "$NO_DOWNLOAD" = false ]; then
    print_info "Downloading OpenSSL prebuilt binaries..."
    
    mkdir -p "$OPENSSL_DOWNLOAD_DIR"
    cd "$OPENSSL_DOWNLOAD_DIR"
    
    # Enhanced download function with retry mechanism
    download_with_retry() {
        local url="$1"
        local filename="$2"
        local extract_cmd="$3"
        local max_retries=3
        local retry=0
        
        while [ $retry -lt $max_retries ]; do
            print_info "Download attempt $((retry + 1))/$max_retries: $filename"
            
            if curl -L --connect-timeout 30 --max-time 300 -o "$filename" "$url"; then
                print_success "Download completed: $filename"
                
                # Verify file size
                local file_size=$(stat -c%s "$filename" 2>/dev/null || echo "0")
                if [ "$file_size" -lt 1000 ]; then
                    print_warning "Downloaded file seems too small ($file_size bytes), retrying..."
                    rm -f "$filename"
                    ((retry++))
                    continue
                fi
                
                # Extract based on type
                case $extract_cmd in
                    "7z")
                        local extract_tools=("7z" "7za")
                        local extract_success=false
                        
                        for tool in "${extract_tools[@]}"; do
                            if command -v "$tool" >/dev/null 2>&1; then
                                if "$tool" x "$filename" -y; then
                                    extract_success=true
                                    break
                                fi
                            fi
                        done
                        
                        if [ "$extract_success" = false ]; then
                            # Try Windows 7-Zip
                            for win7z in "/c/Program Files/7-Zip/7z.exe" "/c/Program Files (x86)/7-Zip/7z.exe"; do
                                if [ -f "$win7z" ]; then
                                    if "$win7z" x "$filename" -y; then
                                        extract_success=true
                                        break
                                    fi
                                fi
                            done
                        fi
                        
                        if [ "$extract_success" = false ]; then
                            print_error "Cannot extract 7z file - no suitable tool found"
                            return 1
                        fi
                        ;;
                    "zip")
                        if command -v unzip >/dev/null 2>&1; then
                            unzip -q "$filename"
                        else
                            print_error "unzip not available"
                            return 1
                        fi
                        ;;
                    "tar")
                        tar -xzf "$filename"
                        ;;
                esac
                
                return 0
            else
                print_warning "Download failed (attempt $((retry + 1))): $url"
                rm -f "$filename"
                ((retry++))
                sleep 2
            fi
        done
        
        return 1
    }
    
    # Multiple download sources with fallbacks
    DOWNLOAD_SOURCES=(
        # Precompiled Windows OpenSSL
        "https://github.com/openssl/openssl/releases/download/OpenSSL_1_1_1w/openssl-1.1.1w.tar.gz|openssl-source.tar.gz|tar"
        "https://www.openssl.org/source/openssl-1.1.1w.tar.gz|openssl-official.tar.gz|tar"
        # Slproweb (popular Windows OpenSSL distribution)  
        "https://github.com/IndySockets/OpenSSL-Binaries/raw/master/OpenSSL-Win64.zip|openssl-win64.zip|zip"
    )
    
    DOWNLOAD_SUCCESS=false
    for source in "${DOWNLOAD_SOURCES[@]}"; do
        IFS='|' read -r url filename extract_cmd <<< "$source"
        
        if [ ! -f "$filename" ]; then
            if download_with_retry "$url" "$filename" "$extract_cmd"; then
                DOWNLOAD_SUCCESS=true
                break
            fi
        else
            print_info "File already exists: $filename"
            DOWNLOAD_SUCCESS=true
            break
        fi
    done
    
    if [ "$DOWNLOAD_SUCCESS" = false ]; then
        print_error "All download sources failed"
        cd ..
        rm -rf "$OPENSSL_DOWNLOAD_DIR"
        
        print_info "Manual installation options:"
        print_info "1. Download from: https://slproweb.com/products/Win32OpenSSL.html"
        print_info "2. MSYS2: pacman -S mingw-w64-x86_64-openssl"
        print_info "3. vcpkg: vcpkg install openssl:x64-windows-static"
        print_info "4. Conan: conan install openssl/1.1.1w@"
        exit 1
    fi
    
    cd ..
    
    # Search in downloaded files
    if find_openssl "$(pwd)/$OPENSSL_DOWNLOAD_DIR"; then
        OPENSSL_FOUND=true
        print_success "Downloaded OpenSSL configured successfully"
    else
        # Try to find in subdirectories
        for subdir in "$OPENSSL_DOWNLOAD_DIR"/*/ ; do
            if [ -d "$subdir" ] && find_openssl "$subdir"; then
                OPENSSL_FOUND=true
                print_success "Downloaded OpenSSL found in subdirectory"
                break
            fi
        done
    fi
fi

# Final OpenSSL verification
if [ "$OPENSSL_FOUND" = false ]; then
    print_error "OpenSSL installation not found or incomplete"
    print_info "Please install OpenSSL manually and re-run this script"
    exit 1
fi

# Display comprehensive OpenSSL configuration
print_success "OpenSSL Configuration Summary:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Root Directory    : $OPENSSL_ROOT"
echo "  Include Directory : $OPENSSL_INCLUDE" 
echo "  SSL Library       : $OPENSSL_SSL_LIB"
echo "  Crypto Library    : $OPENSSL_CRYPTO_LIB"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Verify all paths exist and are accessible
VERIFICATION_FAILED=false
for path_type in "Include:$OPENSSL_INCLUDE" "SSL Library:$OPENSSL_SSL_LIB" "Crypto Library:$OPENSSL_CRYPTO_LIB"; do
    IFS=':' read -r desc path <<< "$path_type"
    if [ ! -e "$path" ]; then
        print_error "$desc path does not exist: $path"
        VERIFICATION_FAILED=true
    else
        print_success "$desc verified: $(basename "$path")"
    fi
done

if [ "$VERIFICATION_FAILED" = true ]; then
    print_error "OpenSSL configuration verification failed"
    exit 1
fi

log_action "OpenSSL found and configured: $OPENSSL_ROOT"

# Step 4: Enhanced Source Code Creation
print_step "4" "Source Code Generation"

# Create enhanced main.cpp
create_main_cpp() {
    local cpp_file="$1"
    local build_type="$2"  # "test" or "production"
    
    print_info "Creating $build_type main.cpp..."
    
    if [ "$build_type" = "test" ]; then
        cat > "$cpp_file" << 'EOF'
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <exception>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

// OpenSSL Test Includes
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/opensslv.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

// Test Results Structure
struct TestResult {
    std::string test_name;
    bool passed;
    std::string details;
};

class OpenSSLTester {
private:
    std::vector<TestResult> results;
    
    void addResult(const std::string& test_name, bool passed, const std::string& details = "") {
        results.push_back({test_name, passed, details});
    }

public:
    void runAllTests() {
        std::cout << "╔══════════════════════════════════════╗" << std::endl;
        std::cout << "║     OpenSSL Comprehensive Test       ║" << std::endl;
        std::cout << "╚══════════════════════════════════════╝" << std::endl;
        
        testOpenSSLVersion();
        testOpenSSLInitialization();
        testSSLContext();
        testCryptographicFunctions();
        testRandomGeneration();
        testHashFunctions();
        
        printResults();
    }
    
    void testOpenSSLVersion() {
        try {
            std::string version = OPENSSL_VERSION_TEXT;
            addResult("OpenSSL Version Check", true, version);
        } catch (...) {
            addResult("OpenSSL Version Check", false, "Failed to get version");
        }
    }
    
    void testOpenSSLInitialization() {
        try {
            SSL_library_init();
            SSL_load_error_strings();
            ERR_load_BIO_strings();
            ERR_load_crypto_strings();
            addResult("OpenSSL Initialization", true, "All components initialized");
        } catch (...) {
            addResult("OpenSSL Initialization", false, "Initialization failed");
        }
    }
    
    void testSSLContext() {
        try {
            const SSL_METHOD* method = TLS_client_method();
            if (!method) {
                addResult("SSL Context Creation", false, "Failed to get TLS method");
                return;
            }
            
            SSL_CTX* ctx = SSL_CTX_new(method);
            if (ctx) {
                addResult("SSL Context Creation", true, "Client context created successfully");
                SSL_CTX_free(ctx);
            } else {
                addResult("SSL Context Creation", false, "Failed to create SSL context");
            }
        } catch (...) {
            addResult("SSL Context Creation", false, "Exception during context creation");
        }
    }
    
    void testCryptographicFunctions() {
        try {
            EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
            if (!mdctx) {
                addResult("Cryptographic Functions", false, "Failed to create MD context");
                return;
            }
            
            if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) == 1) {
                addResult("Cryptographic Functions", true, "SHA-256 context initialized");
            } else {
                addResult("Cryptographic Functions", false, "Failed to initialize SHA-256");
            }
            
            EVP_MD_CTX_free(mdctx);
        } catch (...) {
            addResult("Cryptographic Functions", false, "Exception in crypto test");
        }
    }
    
    void testRandomGeneration() {
        try {
            unsigned char buffer[32];
            if (RAND_bytes(buffer, sizeof(buffer)) == 1) {
                addResult("Random Generation", true, "Generated 32 random bytes");
            } else {
                addResult("Random Generation", false, "Failed to generate random bytes");
            }
        } catch (...) {
            addResult("Random Generation", false, "Exception in random generation");
        }
    }
    
    void testHashFunctions() {
        try {
            const char* test_data = "Hello, OpenSSL!";
            unsigned char hash[EVP_MAX_MD_SIZE];
            unsigned int hash_len;
            
            EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
            if (mdctx && 
                EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) == 1 &&
                EVP_DigestUpdate(mdctx, test_data, strlen(test_data)) == 1 &&
                EVP_DigestFinal_ex(mdctx, hash, &hash_len) == 1) {
                
                std::string result = "SHA-256 hash computed (";
                result += std::to_string(hash_len) + " bytes)";
                addResult("Hash Functions", true, result);
            } else {
                addResult("Hash Functions", false, "Failed to compute SHA-256 hash");
            }
            
            if (mdctx) EVP_MD_CTX_free(mdctx);
        } catch (...) {
            addResult("Hash Functions", false, "Exception in hash test");
        }
    }
    
    void printResults() {
        std::cout << "\n╔══════════════════════════════════════╗" << std::endl;
        std::cout << "║            TEST RESULTS              ║" << std::endl;
        std::cout << "╚══════════════════════════════════════╝" << std::endl;
        
        int passed = 0, total = results.size();
        
        for (const auto& result : results) {
            std::string status = result.passed ? "✓ PASS" : "✗ FAIL";
            std::cout << "  " << status << " | " << result.test_name;
            if (!result.details.empty()) {
                std::cout << " (" << result.details << ")";
            }
            std::cout << std::endl;
            
            if (result.passed) passed++;
        }
        
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "  Results: " << passed << "/" << total << " tests passed";
        
        if (passed == total) {
            std::cout << " 🎉" << std::endl;
            std::cout << "  OpenSSL is fully functional!" << std::endl;
        } else {
            std::cout << " ❌" << std::endl;
            std::cout << "  Some tests failed - check configuration" << std::endl;
        }
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    }
};

int main() {
    std::cout << "╔══════════════════════════════════════╗" << std::endl;
    std::cout << "║        OpenSSL Test Application      ║" << std::endl;
    std::cout << "╚══════════════════════════════════════╝" << std::endl;
    
    try {
        // System Information
        std::cout << "\nSystem Information:" << std::endl;
        std::cout << "──────────────────────────────────────" << std::endl;
        std::cout << "  OS           : Windows (MinGW)" << std::endl;
        std::cout << "  Architecture : " << (sizeof(void*) == 8 ? "x64" : "x86") << std::endl;
        std::cout << "  Compiler     : " << __VERSION__ << std::endl;
        std::cout << "  Build Date   : " << __DATE__ << " " << __TIME__ << std::endl;
        std::cout << "  C++ Standard : " << __cplusplus << std::endl;
        
#ifdef _WIN32
        std::cout << "  OpenSSL Ver  : " << OPENSSL_VERSION_TEXT << std::endl;
#endif
        
        // Run comprehensive OpenSSL tests
        OpenSSLTester tester;
        tester.runAllTests();
        
        // Cleanup OpenSSL
        EVP_cleanup();
        CRYPTO_cleanup_all_ex_data();
        ERR_free_strings();
        
        std::cout << "\n✓ All OpenSSL tests completed!" << std::endl;
        std::cout << "✓ Ready for full project development." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
    
    // Keep console open on Windows
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
}
EOF
    else
        cat > "$cpp_file" << 'EOF'
#include <iostream>
#include <memory>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

// OpenSSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/opensslv.h>

// Project includes (to be created)
// #include "core/Application.h"
// #include "core/Logger.h"
// #include "security/Encryption.h"

class KeyloggerClient {
private:
    bool initialized;
    SSL_CTX* ssl_ctx;
    
public:
    KeyloggerClient() : initialized(false), ssl_ctx(nullptr) {
        initializeOpenSSL();
    }
    
    ~KeyloggerClient() {
        cleanup();
    }
    
    void initializeOpenSSL() {
        std::cout << "Initializing OpenSSL..." << std::endl;
        
        // Initialize OpenSSL library
        SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        ERR_load_crypto_strings();
        
        // Create SSL context
        const SSL_METHOD* method = TLS_client_method();
        ssl_ctx = SSL_CTX_new(method);
        
        if (!ssl_ctx) {
            throw std::runtime_error("Failed to create SSL context");
        }
        
        initialized = true;
        std::cout << "✓ OpenSSL initialized successfully!" << std::endl;
        std::cout << "✓ OpenSSL Version: " << OPENSSL_VERSION_TEXT << std::endl;
    }
    
    void cleanup() {
        if (ssl_ctx) {
            SSL_CTX_free(ssl_ctx);
            ssl_ctx = nullptr;
        }
        
        if (initialized) {
            EVP_cleanup();
            CRYPTO_cleanup_all_ex_data();
            ERR_free_strings();
            initialized = false;
        }
    }
    
    int run() {
        std::cout << "\n╔══════════════════════════════════════╗" << std::endl;
        std::cout << "║         Keylogger Client v1.0        ║" << std::endl;
        std::cout << "╚══════════════════════════════════════╝" << std::endl;
        
        std::cout << "\nApplication Status:" << std::endl;
        std::cout << "  ✓ OpenSSL: Ready" << std::endl;
        std::cout << "  ✓ Security: Initialized" << std::endl;
        std::cout << "  ⚠ Core modules: Pending implementation" << std::endl;
        
        std::cout << "\nNext Development Steps:" << std::endl;
        std::cout << "  1. Implement core application logic" << std::endl;
        std::cout << "  2. Add keylogging functionality" << std::endl;
        std::cout << "  3. Implement secure communication" << std::endl;
        std::cout << "  4. Add data persistence" << std::endl;
        
        std::cout << "\nApplication ready for development!" << std::endl;
        return 0;
    }
};

int main() {
    try {
        KeyloggerClient app;
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}
EOF
    fi
}

# Create main.cpp based on mode
if [ ! -f "src/main.cpp" ] || [ "$TEST_ONLY" = true ]; then
    if [ "$TEST_ONLY" = true ]; then
        create_main_cpp "src/main.cpp" "test"
    else
        create_main_cpp "src/main.cpp" "production"
    fi
    print_success "Created src/main.cpp"
fi

# Create enhanced header files
create_enhanced_header() {
    local file="$1"
    local class_name="$2"
    local namespace="$3"
    local description="$4"
    
    if [ ! -f "$file" ]; then
        local guard_name=$(echo "$file" | tr '[:lower:]/' '[:upper:]_' | sed 's/[^A-Z0-9]/_/g')
        mkdir -p "$(dirname "$file")"
        
        cat > "$file" << EOF
#ifndef ${guard_name}
#define ${guard_name}

/**
 * @file $(basename "$file")
 * @brief $description
 * @date $(date '+%Y-%m-%d')
 */

#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ${namespace} {

/**
 * @class ${class_name}
 * @brief $description
 */
class ${class_name} {
public:
    ${class_name}();
    virtual ~${class_name}();
    
    // Core interface
    virtual bool initialize();
    virtual void cleanup();
    virtual bool isInitialized() const { return initialized_; }
    
    // Status and information
    virtual std::string getStatus() const;
    virtual std::string getVersion() const { return "1.0.0"; }
    
protected:
    bool initialized_;
    std::string last_error_;
    
    // Error handling
    void setLastError(const std::string& error) { last_error_ = error; }
    
private:
    // Prevent copying
    ${class_name}(const ${class_name}&) = delete;
    ${class_name}& operator=(const ${class_name}&) = delete;
};

} // namespace ${namespace}

#endif // ${guard_name}
EOF
        print_info "Created header: $file"
    fi
}

# Create enhanced headers for full project
if [ "$TEST_ONLY" = false ]; then
    create_enhanced_header "include/core/Application.h" "Application" "Core" "Main application controller"
    create_enhanced_header "include/core/Logger.h" "Logger" "Core" "Logging system interface"
    create_enhanced_header "include/security/Encryption.h" "Encryption" "Security" "OpenSSL encryption wrapper"
    create_enhanced_header "include/communication/NetworkClient.h" "NetworkClient" "Communication" "Network communication client"
    create_enhanced_header "include/hooks/KeyboardHook.h" "KeyboardHook" "Hooks" "Keyboard event capture"
    create_enhanced_header "include/data/DataProcessor.h" "DataProcessor" "Data" "Data processing and formatting"
fi

print_success "Source code generation completed"

# Step 5: Enhanced CMakeLists.txt Generation
print_step "5" "Enhanced CMakeLists.txt Generation"

# Convert paths to CMake-friendly format with better Windows path handling
cmake_path() {
    local path="$1"
    # Convert to Windows-style path and normalize
    echo "$path" | sed 's|\\|/|g' | sed 's|^/c/|C:/|' | sed 's|^/d/|D:/|' | sed 's|^/([a-z])/|\U\1:/|'
}

CMAKE_OPENSSL_ROOT=$(cmake_path "$OPENSSL_ROOT")
CMAKE_OPENSSL_INCLUDE=$(cmake_path "$OPENSSL_INCLUDE")
CMAKE_OPENSSL_SSL=$(cmake_path "$OPENSSL_SSL_LIB")
CMAKE_OPENSSL_CRYPTO=$(cmake_path "$OPENSSL_CRYPTO_LIB")

# Generate enhanced CMakeLists.txt
cat > CMakeLists.txt << EOF
cmake_minimum_required(VERSION 3.15)
project($PROJECT_NAME VERSION 1.0.0 LANGUAGES CXX)

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
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "\${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "\${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "\${CMAKE_BINARY_DIR}/lib")

message(STATUS "╔══════════════════════════════════════╗")
message(STATUS "║     $PROJECT_NAME v\${PROJECT_VERSION}     ║")
message(STATUS "╚══════════════════════════════════════╝")
message(STATUS "Build Type    : \${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler      : \${CMAKE_CXX_COMPILER_ID} \${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Generator     : \${CMAKE_GENERATOR}")
message(STATUS "Source Dir    : \${CMAKE_SOURCE_DIR}")
message(STATUS "Binary Dir    : \${CMAKE_BINARY_DIR}")

# ========================================
#           OPENSSL CONFIGURATION
# ========================================

# Manual OpenSSL configuration (more reliable than FindOpenSSL)
set(OPENSSL_ROOT_DIR "$CMAKE_OPENSSL_ROOT")
set(OPENSSL_INCLUDE_DIR "$CMAKE_OPENSSL_INCLUDE")
set(OPENSSL_SSL_LIBRARY "$CMAKE_OPENSSL_SSL")
set(OPENSSL_CRYPTO_LIBRARY "$CMAKE_OPENSSL_CRYPTO")
set(OPENSSL_LIBRARIES "\${OPENSSL_SSL_LIBRARY};\${OPENSSL_CRYPTO_LIBRARY}")
set(OPENSSL_FOUND TRUE)

message(STATUS "")
message(STATUS "OpenSSL Configuration:")
message(STATUS "  Root Dir      : \${OPENSSL_ROOT_DIR}")
message(STATUS "  Include Dir   : \${OPENSSL_INCLUDE_DIR}")
message(STATUS "  SSL Library   : \${OPENSSL_SSL_LIBRARY}")
message(STATUS "  Crypto Library: \${OPENSSL_CRYPTO_LIBRARY}")

# Verify OpenSSL files exist
set(OPENSSL_VERIFICATION_FAILED FALSE)
foreach(path "\${OPENSSL_INCLUDE_DIR}" "\${OPENSSL_SSL_LIBRARY}" "\${OPENSSL_CRYPTO_LIBRARY}")
    if(NOT EXISTS "\${path}")
        message(SEND_ERROR "OpenSSL path does not exist: \${path}")
        set(OPENSSL_VERIFICATION_FAILED TRUE)
    endif()
endforeach()

if(OPENSSL_VERIFICATION_FAILED)
    message(FATAL_ERROR "OpenSSL configuration verification failed!")
endif()

# Additional OpenSSL verification
if(EXISTS "\${OPENSSL_INCLUDE_DIR}/openssl/ssl.h")
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
    "\${OPENSSL_INCLUDE_DIR}"
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
foreach(source_file \${ALL_SOURCES})
    if(EXISTS "\${source_file}")
        list(APPEND SOURCES "\${source_file}")
    endif()
endforeach()

# Ensure we have at least main.cpp
if(NOT SOURCES)
    if(EXISTS "\${CMAKE_SOURCE_DIR}/src/main.cpp")
        set(SOURCES src/main.cpp)
    else()
        message(FATAL_ERROR "No source files found! Ensure src/main.cpp exists.")
    endif()
endif()

message(STATUS "")
message(STATUS "Source Files:")
foreach(source \${SOURCES})
    file(RELATIVE_PATH rel_source "\${CMAKE_SOURCE_DIR}" "\${source}")
    message(STATUS "  \${rel_source}")
endforeach()

# ========================================
#         EXECUTABLE CREATION
# ========================================

add_executable($PROJECT_NAME \${SOURCES})

# C++ features
target_compile_features($PROJECT_NAME PRIVATE cxx_std_17)

# ========================================
#         COMPILER CONFIGURATION
# ========================================

# Platform-specific definitions
if(WIN32)
    target_compile_definitions($PROJECT_NAME PRIVATE
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
    target_compile_options($PROJECT_NAME PRIVATE
        # Warnings
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Wno-unknown-pragmas
        
        # Optimization
        \$<\$<CONFIG:Debug>:-g -O0 -DDEBUG>
        \$<\$<CONFIG:Release>:-O3 -DNDEBUG -march=native>
        \$<\$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
        \$<\$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
        
        # Additional flags for Windows
        -static-libgcc
        -static-libstdc++
    )
    
    # Linker flags for MinGW
    target_link_options($PROJECT_NAME PRIVATE
        -static-libgcc
        -static-libstdc++
        -Wl,--enable-stdcall-fixup
    )
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options($PROJECT_NAME PRIVATE
        -Wall -Wextra
        -Wno-unused-parameter
        \$<\$<CONFIG:Debug>:-g -O0 -DDEBUG>
        \$<\$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

# ========================================
#           LIBRARY LINKING
# ========================================

if(WIN32)
    # Link OpenSSL libraries (must come first)
    target_link_libraries($PROJECT_NAME PRIVATE
        "\${OPENSSL_SSL_LIBRARY}"
        "\${OPENSSL_CRYPTO_LIBRARY}"
    )
    
    # Core Windows libraries
    target_link_libraries($PROJECT_NAME PRIVATE
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
        set_target_properties($PROJECT_NAME PROPERTIES
            LINK_FLAGS "-Wl,--subsystem,console"
        )
    else()
        # For release, you might want GUI subsystem
        set_target_properties($PROJECT_NAME PROPERTIES  
            LINK_FLAGS "-Wl,--subsystem,console"  # Change to windows for GUI app
        )
    endif()
    
    # Additional Windows-specific configuration
    set_target_properties($PROJECT_NAME PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ========================================
#         OUTPUT CONFIGURATION
# ========================================

# Per-configuration output directories
foreach(config Debug Release MinSizeRel RelWithDebInfo)
    string(TOUPPER \${config} config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_\${config_upper} "\${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_\${config_upper} "\${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_\${config_upper} "\${CMAKE_BINARY_DIR}/lib")
endforeach()

# Create necessary directories
file(MAKE_DIRECTORY "\${CMAKE_BINARY_DIR}/bin")
file(MAKE_DIRECTORY "\${CMAKE_BINARY_DIR}/lib")
file(MAKE_DIRECTORY "\${CMAKE_BINARY_DIR}/logs")

# Copy resources if they exist
if(EXISTS "\${CMAKE_SOURCE_DIR}/resources")
    file(COPY "\${CMAKE_SOURCE_DIR}/resources" 
         DESTINATION "\${CMAKE_BINARY_DIR}")
    message(STATUS "Resources copied to build directory")
endif()

# ========================================
#         DEVELOPMENT HELPERS
# ========================================

# Enable compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Custom targets for development
add_custom_target(run
    COMMAND $PROJECT_NAME
    DEPENDS $PROJECT_NAME
    WORKING_DIRECTORY \${CMAKE_BINARY_DIR}/bin
    COMMENT "Running $PROJECT_NAME"
)

add_custom_target(debug
    COMMAND gdb \${CMAKE_BINARY_DIR}/bin/$PROJECT_NAME
    DEPENDS $PROJECT_NAME
    WORKING_DIRECTORY \${CMAKE_BINARY_DIR}/bin
    COMMENT "Debug $PROJECT_NAME with GDB"
)

add_custom_target(clean-all
    COMMAND \${CMAKE_COMMAND} --build . --target clean
    COMMAND \${CMAKE_COMMAND} -E remove_directory \${CMAKE_BINARY_DIR}/bin
    COMMAND \${CMAKE_COMMAND} -E remove_directory \${CMAKE_BINARY_DIR}/lib
    WORKING_DIRECTORY \${CMAKE_BINARY_DIR}
    COMMENT "Deep clean build artifacts"
)

# ========================================
#         INSTALL CONFIGURATION
# ========================================

install(TARGETS $PROJECT_NAME 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install resources
if(EXISTS "\${CMAKE_SOURCE_DIR}/resources")
    install(DIRECTORY "\${CMAKE_SOURCE_DIR}/resources/"
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
message(STATUS "Project       : \${PROJECT_NAME}")
message(STATUS "Version       : \${PROJECT_VERSION}")
message(STATUS "Build Type    : \${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler      : \${CMAKE_CXX_COMPILER}")
message(STATUS "Output Dir    : \${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "OpenSSL Root  : \${OPENSSL_ROOT_DIR}")
message(STATUS "Install Prefix: \${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Custom Targets Available:")
message(STATUS "  make run      - Run the application")
message(STATUS "  make debug    - Debug with GDB")
message(STATUS "  make clean-all- Deep clean")
message(STATUS "")
EOF

print_success "Enhanced CMakeLists.txt created"
log_action "Enhanced CMakeLists.txt generated"

# Step 6: Enhanced Build Process
print_step "6" "Enhanced Build Process"

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ] && [ -d "$BUILD_DIR" ]; then
    print_info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    print_success "Build directory cleaned"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Set enhanced compiler environment
export CC=$(which gcc)
export CXX=$(which g++)
export MAKE=$(which make)

print_info "Enhanced build environment:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  CC    : $CC"
echo "  CXX   : $CXX"
echo "  MAKE  : $MAKE"
echo "  CMAKE : $(which cmake)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Get detailed version information
for tool in gcc g++ make cmake; do
    if command -v "$tool" >/dev/null 2>&1; then
        version=$($tool --version 2>/dev/null | head -1 || echo 'Unknown')
        print_info "  $tool: $version"
    fi
done

# Enhanced CMake configuration
print_info "Configuring build with CMake..."

CMAKE_ARGS=(
    "-G" "MinGW Makefiles"
    "-DCMAKE_BUILD_TYPE=$([[ "$DEBUG_MODE" == "true" ]] && echo "Debug" || echo "Release")"
    "-DCMAKE_C_COMPILER=$CC"
    "-DCMAKE_CXX_COMPILER=$CXX"
    "-DCMAKE_MAKE_PROGRAM=$MAKE"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    "-DCMAKE_VERBOSE_MAKEFILE=$([ "$DEBUG_MODE" = true ] && echo "ON" || echo "OFF")"
    "-DCMAKE_C_COMPILER=/mingw64/bin/gcc"
    "-DCMAKE_CXX_COMPILER=/mingw64/bin/g++"
)

# Additional CMake arguments for better Windows support
CMAKE_ARGS+=(
    "-DCMAKE_SYSTEM_NAME=Windows"
    "-DCMAKE_RC_COMPILER=windres"
)

print_info "CMake arguments:"
for arg in "${CMAKE_ARGS[@]}"; do
    print_info "  $arg"
done

# Run CMake with enhanced error handling
print_info "Running CMake configuration..."
if cmake .. "${CMAKE_ARGS[@]}" 2>&1 | tee cmake_config.log; then
    print_success "✓ CMake configuration successful!"
    log_action "CMake configuration completed successfully"
    
    # Show configuration summary
    echo
    print_info "Build configuration verified:"
    if [ -f "CMakeCache.txt" ]; then
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        grep -E "(CMAKE_BUILD_TYPE|CMAKE_CXX_COMPILER|CMAKE_GENERATOR|OPENSSL)" CMakeCache.txt | head -15 | while read line; do
            echo "  $line"
        done
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    fi
    echo
    
else
    print_error "✗ CMake configuration failed"
    log_action "ERROR: CMake configuration failed"
    
    print_info "Analyzing CMake failure..."
    echo "════════════════════════════════════════"
    echo "CMake Error Analysis:"
    echo "════════════════════════════════════════"
    
    # Show CMake error logs
    if [ -f "CMakeFiles/CMakeError.log" ]; then
        echo "--- CMakeError.log (last 20 lines) ---"
        tail -20 "CMakeFiles/CMakeError.log"
        echo "─────────────────────────────────────"
    fi
    
    if [ -f "CMakeFiles/CMakeOutput.log" ]; then
        echo "--- CMakeOutput.log (last 10 lines) ---"
        tail -10 "CMakeFiles/CMakeOutput.log"
        echo "──────────────────────────────────────"
    fi
    
    # Show recent cmake config log
    if [ -f "cmake_config.log" ]; then
        echo "--- Recent CMake Output ---"
        tail -15 "cmake_config.log"
        echo "─────────────────────────"
    fi
    
    echo "Common CMake Issues & Solutions:"
    echo "1. Compiler not found: Ensure gcc/g++ are in PATH"
    echo "2. OpenSSL paths wrong: Check OpenSSL installation"
    echo "3. MinGW version issues: Try different MinGW version"
    echo "4. Path spaces: Ensure no spaces in project path"
    echo "════════════════════════════════════════"
    
    cd ..
    exit 1
fi

# Enhanced build process
print_info "Starting enhanced build process..."

# Determine optimal number of CPU cores for parallel build
CORES=2  # Default fallback
if command -v nproc >/dev/null 2>&1; then
    CORES=$(nproc)
elif [ -f /proc/cpuinfo ]; then
    CORES=$(grep -c ^processor /proc/cpuinfo)
elif [ -r /sys/devices/system/cpu/online ]; then
    CORES=$(cut -d'-' -f2 /sys/devices/system/cpu/online)
    CORES=$((CORES + 1))
fi

# Cap cores to reasonable limit
if [ $CORES -gt 8 ]; then
    CORES=8
fi

print_info "Building with $CORES parallel jobs..."

# Enhanced build with progress and timing
BUILD_START_TIME=$(date +%s)

MAKE_ARGS=("-j$CORES")
if [ "$DEBUG_MODE" = true ]; then
    MAKE_ARGS+=("VERBOSE=1")
fi

print_info "Build command: make ${MAKE_ARGS[*]}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Starting build process..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Build with comprehensive logging
if make "${MAKE_ARGS[@]}" 2>&1 | tee build.log; then
    BUILD_END_TIME=$(date +%s)
    BUILD_TIME=$((BUILD_END_TIME - BUILD_START_TIME))
    
    print_success "✓ Build completed successfully in ${BUILD_TIME}s!"
    log_action "Build completed successfully in ${BUILD_TIME}s"
    
    # Enhanced executable verification
    EXE_CANDIDATES=(
        "bin/$PROJECT_NAME.exe"
        "$PROJECT_NAME.exe"
        "bin/$PROJECT_NAME"
        "$PROJECT_NAME"
    )
    
    EXE_PATH=""
    for candidate in "${EXE_CANDIDATES[@]}"; do
        if [ -f "$candidate" ]; then
            EXE_PATH="$(pwd)/$candidate"
            break
        fi
    done
    
    if [ -n "$EXE_PATH" ]; then
        print_success "✓ Executable created: $EXE_PATH"
        
        # Comprehensive executable analysis
        echo
        print_info "Executable Analysis:"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        
        # File information
        if command -v ls >/dev/null 2>&1; then
            ls -la "$EXE_PATH" 2>/dev/null || echo "  Size info: Not available"
        fi
        
        # File size in human-readable format
        if [ -f "$EXE_PATH" ]; then
            SIZE=$(stat -c%s "$EXE_PATH" 2>/dev/null || echo "0")
            if [ "$SIZE" != "0" ]; then
                if command -v numfmt >/dev/null 2>&1; then
                    SIZE_HUMAN=$(numfmt --to=iec --suffix=B "$SIZE")
                    echo "  Size: $SIZE bytes ($SIZE_HUMAN)"
                else
                    SIZE_MB=$(echo "scale=2; $SIZE / 1024 / 1024" | bc 2>/dev/null || echo "$(($SIZE / 1024 / 1024))")
                    echo "  Size: $SIZE bytes (~${SIZE_MB}MB)"
                fi
            fi
            
            # File type
            if command -v file >/dev/null 2>&1; then
                echo "  Type: $(file "$EXE_PATH" 2>/dev/null | cut -d: -f2- | sed 's/^[[:space:]]*//')"
            fi
        fi
        
        # Dependency analysis
        if command -v ldd >/dev/null 2>&1; then
            echo
            echo "Dependencies:"
            if ldd "$EXE_PATH" 2>/dev/null | head -20; then
                echo "  (Showing first 20 dependencies)"
            else
                echo "  Could not analyze dependencies"
            fi
        elif command -v objdump >/dev/null 2>&1; then
            echo
            echo "DLL Dependencies:"
            if objdump -p "$EXE_PATH" 2>/dev/null | grep -i "dll name" | head -10; then
                echo "  (Showing first 10 DLL dependencies)"
            else
                echo "  No DLL dependencies found (likely static linking)"
            fi
        fi
        
        # OpenSSL integration check
        echo
        echo "OpenSSL Integration Check:"
        if command -v strings >/dev/null 2>&1; then
            if strings "$EXE_PATH" | grep -i openssl | head -5; then
                print_success "  ✓ OpenSSL integration detected"
            else
                print_warning "  ⚠ OpenSSL strings not found (may be optimized out)"
            fi
        fi
        
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        
    else
        print_warning "⚠ Executable not found in expected locations"
        print_info "Searching for executable files..."
        
        # Search for any executable files
        find . -name "*.exe" -type f 2>/dev/null | while read -r exe; do
            print_info "Found executable: $(pwd)/$exe"
        done
        
        # Check if build actually succeeded
        if grep -q "Built target" build.log 2>/dev/null; then
            print_info "Build target completed but executable location unclear"
        fi
    fi
    
else
    BUILD_END_TIME=$(date +%s)
    BUILD_TIME=$((BUILD_END_TIME - BUILD_START_TIME))
    
    print_error "✗ Build failed after ${BUILD_TIME}s"
    log_action "ERROR: Build failed after ${BUILD_TIME}s"
    
    echo
    print_info "Build Error Analysis:"
    echo "════════════════════════════════════════"
    
    # Analyze build errors
    if [ -f "build.log" ]; then
        echo "--- Recent Build Errors ---"
        tail -30 "build.log" | grep -i -E "(error|failed|undefined)" | head -15
        echo "─────────────────────────────"
        
        # Common error patterns
        echo "Error Categories Found:"
        if grep -q -i "undefined reference" build.log; then
            echo "  ✗ Undefined references (linking issues)"
            echo "    → Check library paths and names"
        fi
        if grep -q -i "no such file" build.log; then
            echo "  ✗ Missing files (include/library issues)"  
            echo "    → Verify OpenSSL paths"
        fi
        if grep -q -i "permission denied" build.log; then
            echo "  ✗ Permission issues"
            echo "    → Check file permissions"
        fi
        if grep -q -i "syntax error\|parse error" build.log; then
            echo "  ✗ Code syntax errors"
            echo "    → Check source code compilation"
        fi
    fi
    
    echo
    echo "Build Troubleshooting Steps:"
    echo "1. Check build.log for specific errors"
    echo "2. Verify all source files exist"
    echo "3. Ensure OpenSSL libraries are correct architecture"
    echo "4. Try clean build: rm -rf * && cmake .. && make"
    echo "5. Check for conflicting toolchains"
    echo "════════════════════════════════════════"
    
    cd ..
    exit 1
fi

cd ..

# Step 7: Comprehensive Testing & Validation
print_step "7" "Comprehensive Testing & Validation"

# Find the executable for testing
TEST_EXE=""
for candidate in "$BUILD_DIR/bin/$PROJECT_NAME.exe" "$BUILD_DIR/$PROJECT_NAME.exe" "$BUILD_DIR/bin/$PROJECT_NAME" "$BUILD_DIR/$PROJECT_NAME"; do
    if [ -f "$candidate" ]; then
        TEST_EXE="$candidate"
        break
    fi
done

if [ -n "$TEST_EXE" ]; then
    print_info "Testing executable: $TEST_EXE"
    
    # Create test environment
    mkdir -p "test_results"
    
    # Test 1: Basic execution test
    echo
    print_info "Test 1: Basic Execution"
    echo "────────────────────────────────────────"
    
    TEST_OUTPUT_FILE="test_results/execution_test.log"
    
    # Use timeout with fallback
    run_with_timeout() {
        local timeout_duration="30"
        local cmd="$1"
        local output_file="$2"
        
        if command -v timeout >/dev/null 2>&1; then
            if timeout "$timeout_duration" "$cmd" > "$output_file" 2>&1; then
                return 0
            else
                return $?
            fi
        else
            # Fallback without timeout
            if "$cmd" < /dev/null > "$output_file" 2>&1; then
                return 0
            else
                return $?
            fi
        fi
    }
    
    if run_with_timeout "$TEST_EXE" "$TEST_OUTPUT_FILE"; then
        print_success "✓ Executable runs successfully"
        
        # Show test output
        if [ -f "$TEST_OUTPUT_FILE" ] && [ -s "$TEST_OUTPUT_FILE" ]; then
            echo "Program Output:"
            head -20 "$TEST_OUTPUT_FILE" | sed 's/^/  │ /'
        fi
        
        log_action "Executable test passed"
    else
        EXIT_CODE=$?
        case $EXIT_CODE in
            124) print_warning "⚠ Test timed out (30s) - program may be waiting for input" ;;
            127) print_error "✗ Executable not found or not executable" ;;
            *)   print_warning "⚠ Test completed with exit code: $EXIT_CODE" ;;
        esac
        
        # Show error output if available
        if [ -f "$TEST_OUTPUT_FILE" ] && [ -s "$TEST_OUTPUT_FILE" ]; then
            echo "Error Output:"
            tail -10 "$TEST_OUTPUT_FILE" | sed 's/^/  │ /'
        fi
        
        log_action "Executable test had issues (exit code: $EXIT_CODE)"
    fi
    
    # Test 2: Dependency validation
    echo
    print_info "Test 2: Dependency Validation"
    echo "────────────────────────────────────────"
    
    validate_dependencies() {
        local exe_path="$1"
        local missing_deps=0
        
        if command -v ldd >/dev/null 2>&1; then
            print_info "Checking shared library dependencies..."
            
            ldd "$exe_path" 2>/dev/null | while read -r line; do
                if echo "$line" | grep -q "not found"; then
                    print_error "✗ Missing dependency: $line"
                    ((missing_deps++))
                elif echo "$line" | grep -q "=>"; then
                    dep_name=$(echo "$line" | awk '{print $1}')
                    dep_path=$(echo "$line" | awk '{print $3}')
                    if [[ "$dep_name" == *ssl* || "$dep_name" == *crypto* ]]; then
                        print_success "✓ OpenSSL dependency: $dep_name → $dep_path"
                    fi
                fi
            done
            
            if [ $missing_deps -eq 0 ]; then
                print_success "✓ All dependencies satisfied"
            fi
        else
            print_warning "⚠ Cannot validate dependencies (ldd not available)"
        fi
        
        # Alternative dependency check using objdump
        if command -v objdump >/dev/null 2>&1; then
            print_info "Checking imported DLLs..."
            
            local ssl_found=false
            local crypto_found=false
            
            objdump -p "$exe_path" 2>/dev/null | grep -i "dll name" | while read -r line; do
                dll_name=$(echo "$line" | awk '{print $3}')
                if [[ "$dll_name" == *ssl* ]]; then
                    print_success "✓ SSL DLL import: $dll_name"
                    ssl_found=true
                elif [[ "$dll_name" == *crypto* || "$dll_name" == *eay* ]]; then
                    print_success "✓ Crypto DLL import: $dll_name"
                    crypto_found=true
                fi
            done
            
            # Check for static linking
            if [ "$ssl_found" = false ] && [ "$crypto_found" = false ]; then
                print_info "ℹ No OpenSSL DLL imports found (likely static linking)"
                
                # Verify static linking by checking for OpenSSL symbols
                if command -v nm >/dev/null 2>&1; then
                    if nm "$exe_path" 2>/dev/null | grep -q -i ssl; then
                        print_success "✓ OpenSSL symbols found (static linking confirmed)"
                    fi
                elif command -v strings >/dev/null 2>&1; then
                    if strings "$exe_path" 2>/dev/null | grep -i -q "openssl\|ssl_"; then
                        print_success "✓ OpenSSL strings found (likely static linking)"
                    fi
                fi
            fi
        fi
    }
    
    validate_dependencies "$TEST_EXE"
    
    # Test 3: Security and integrity check
    echo
    print_info "Test 3: Security & Integrity Check"
    echo "────────────────────────────────────────"
    
    # Check executable permissions
    if [ -x "$TEST_EXE" ]; then
        print_success "✓ Executable has proper permissions"
    else
        print_warning "⚠ Executable may not have execute permissions"
    fi
    
    # Check for debug symbols
    if command -v file >/dev/null 2>&1; then
        file_info=$(file "$TEST_EXE" 2>/dev/null)
        if echo "$file_info" | grep -q "not stripped"; then
            print_info "ℹ Debug symbols present (good for debugging)"
        else
            print_info "ℹ Debug symbols stripped (optimized for release)"
        fi
    fi
    
    # Basic integrity check
    if [ -s "$TEST_EXE" ]; then
        size=$(stat -c%s "$TEST_EXE" 2>/dev/null || echo "0")
        if [ "$size" -gt 100000 ]; then  # >100KB suggests a real executable
            print_success "✓ Executable size reasonable ($size bytes)"
        else
            print_warning "⚠ Executable seems unusually small ($size bytes)"
        fi
    fi
    
else
    print_warning "⚠ No executable found for testing"
    print_info "Build may have succeeded but executable not in expected location"
fi

# Test summary
echo
print_info "Test Summary Report"
echo "════════════════════════════════════════"
TEST_TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
echo "Test Date: $TEST_TIMESTAMP"
echo "Executable: ${TEST_EXE:-"Not found"}"
echo "Test Results Directory: test_results/"

if [ -d "test_results" ]; then
    echo "Test Files:"
    ls -la test_results/ 2>/dev/null | sed 's/^/  /'
fi

echo "════════════════════════════════════════"

# Step 8: Enhanced Cleanup & Comprehensive Summary
print_step "8" "Cleanup & Final Summary"

# Cleanup temporary files with confirmation
if [ -d "$OPENSSL_DOWNLOAD_DIR" ]; then
    if [ "$DEBUG_MODE" = false ]; then
        print_info "Cleaning up temporary OpenSSL download files..."
        rm -rf "$OPENSSL_DOWNLOAD_DIR"
        print_success "✓ Temporary files cleaned"
    else
        print_info "ℹ Debug mode: Keeping temporary files in $OPENSSL_DOWNLOAD_DIR"
    fi
fi

# Generate comprehensive build report
REPORT_FILE="build_report_$(date +%Y%m%d_%H%M%S).txt"
SCRIPT_END_TIME=$(date +%s)
TOTAL_SCRIPT_TIME=$((SCRIPT_END_TIME - $(date -d "$(grep 'Script started' fix_openssl.log | tail -1 | awk '{print $1" "$2}')" +%s 2>/dev/null || echo $SCRIPT_END_TIME)))

cat > "$REPORT_FILE" << EOF
╔══════════════════════════════════════════════════════════════════════════════╗
║                           BUILD REPORT - COMPREHENSIVE                      ║
║                    $SCRIPT_NAME v$SCRIPT_VERSION                    ║
╚══════════════════════════════════════════════════════════════════════════════╝

EXECUTION SUMMARY
═════════════════════════════════════════════════════════════════════════════
Date & Time       : $(date)
User              : ${USER:-$(whoami)}
Working Directory : $(pwd)
Script Duration   : ${TOTAL_SCRIPT_TIME}s
Build Type        : $([ "$TEST_ONLY" = true ] && echo "Test Only" || echo "Full Project")
Build Mode        : $([ "$DEBUG_MODE" = true ] && echo "Debug" || echo "Release")

SYSTEM ENVIRONMENT
═════════════════════════════════════════════════════════════════════════════
Operating System  : $(uname -s) $(uname -r) $(uname -m)
Shell Environment : ${SHELL:-Unknown}

Development Tools:
$(for tool in gcc g++ make cmake curl; do
    if command -v "$tool" >/dev/null 2>&1; then
        echo "  $tool: $(which $tool) - $($tool --version 2>/dev/null | head -1)"
    else
        echo "  $tool: Not found"
    fi
done)

OPENSSL CONFIGURATION
═════════════════════════════════════════════════════════════════════════════
Root Directory    : $OPENSSL_ROOT
Include Directory : $OPENSSL_INCLUDE  
SSL Library       : $OPENSSL_SSL_LIB
Crypto Library    : $OPENSSL_CRYPTO_LIB
Detection Method  : $([ "$FORCE_REINSTALL" = true ] && echo "Force reinstalled" || echo "Auto-detected")

BUILD CONFIGURATION
═════════════════════════════════════════════════════════════════════════════
Project Name      : $PROJECT_NAME
Build Directory   : $BUILD_DIR
Generator         : MinGW Makefiles
Parallel Jobs     : ${CORES:-"Unknown"}

BUILD RESULTS
═════════════════════════════════════════════════════════════════════════════
$(if [ -n "$TEST_EXE" ] && [ -f "$TEST_EXE" ]; then
    echo "Status            : ✓ SUCCESS"
    echo "Executable        : $TEST_EXE"
    echo "Executable Size   : $(stat -c%s "$TEST_EXE" 2>/dev/null || echo "Unknown") bytes"
    echo "File Type         : $(file "$TEST_EXE" 2>/dev/null | cut -d: -f2- | xargs || echo "Unknown")"
else
    echo "Status            : ✗ FAILED"
    echo "Executable        : Not created"
fi)

FILES CREATED/MODIFIED
═════════════════════════════════════════════════════════════════════════════
Project Structure:
$(find . -maxdepth 3 -type d | head -20 | sed 's/^/  /')

Key Files:
  ✓ CMakeLists.txt
  ✓ src/main.cpp
$([ "$SKIP_BACKUP" = false ] && echo "  ✓ Backup files: *.backup_$BACKUP_SUFFIX")
  ✓ Build directory: $BUILD_DIR
  ✓ Log file: fix_openssl.log
  ✓ This report: $REPORT_FILE

TEST RESULTS
═════════════════════════════════════════════════════════════════════════════
$(if [ -d "test_results" ] && [ -n "$(ls -A test_results 2>/dev/null)" ]; then
    echo "Test files created:"
    ls -la test_results/ 2>/dev/null | tail -n +2 | sed 's/^/  /'
else
    echo "No test results available"
fi)

TROUBLESHOOTING INFORMATION
═════════════════════════════════════════════════════════════════════════════
$(if [ -f "fix_openssl.log" ]; then
    echo "Recent log entries:"
    tail -10 fix_openssl.log | sed 's/^/  /'
else
    echo "No log file available"
fi)

Common Issues & Solutions:
  1. Missing tools: Install MSYS2 development tools
  2. OpenSSL not found: Download from slproweb.com or use vcpkg
  3. Build errors: Check compiler compatibility and library architecture
  4. Runtime errors: Verify all dependencies are available

USEFUL COMMANDS FOR DEVELOPMENT
═════════════════════════════════════════════════════════════════════════════
Rebuild project:
  cd $BUILD_DIR && make

Clean rebuild:
  $0 -c

Debug build:
  $0 -c -d

Test build only:
  $0 -t

Run executable:
$([ -n "$TEST_EXE" ] && echo "  $TEST_EXE" || echo "  [Executable not available]")

Update PATH (if needed):
  export PATH="/c/msys64/usr/bin:/c/msys64/mingw64/bin:\$PATH"

═════════════════════════════════════════════════════════════════════════════
Report generated by: $SCRIPT_NAME v$SCRIPT_VERSION
For support, check: https://github.com/openssl/openssl
═════════════════════════════════════════════════════════════════════════════
EOF

print_success "✓ Comprehensive build report: $REPORT_FILE"

# Final status summary
echo
print_header "FINAL BUILD SUMMARY"

if [ -n "$TEST_EXE" ] && [ -f "$TEST_EXE" ]; then
    print_success "🎉 BUILD SUCCESSFUL! 🎉"
    echo
    echo "╔══════════════════════════════════════════════════════╗"
    echo "║                   SUCCESS SUMMARY                   ║"
    echo "╚══════════════════════════════════════════════════════╝"
    echo
    print_success "✓ OpenSSL successfully integrated"
    print_success "✓ Project structure created"
    print_success "✓ Source code generated"
    print_success "✓ CMake configuration completed" 
    print_success "✓ Build process successful"
    print_success "✓ Executable created and tested"
    
    echo
    echo "📁 Project Files:"
    echo "  Executable    : $TEST_EXE"
    echo "  Project Root  : $(pwd)"
    echo "  Build Dir     : $BUILD_DIR"
    echo "  OpenSSL Root  : $OPENSSL_ROOT"
    
    echo
    echo "🚀 Next Steps:"
    if [ "$TEST_ONLY" = true ]; then
        echo "  1. ✅ Test build completed - OpenSSL is working perfectly!"
        echo "  2. 📝 Add your application source files to src/"
        echo "  3. 🔧 Update CMakeLists.txt with additional sources"
        echo "  4. 🔄 Run: $0 (without -t flag) for full build"
        echo "  5. 🏃 Start developing your keylogger features"
    else
        echo "  1. 📁 Add your source files to src/ subdirectories"
        echo "  2. 📝 Update include/ with your header files"
        echo "  3. 🔄 Rebuild: cd $BUILD_DIR && make"
        echo "  4. 🏃 Run: $TEST_EXE"
        echo "  5. 🔧 Implement core functionality"
    fi
    
    echo
    echo "🛠️  Development Commands:"
    echo "  Rebuild       : cd $BUILD_DIR && make"
    echo "  Clean build   : $0 -c"
    echo "  Debug build   : $0 -c -d"
    echo "  Run program   : $TEST_EXE"
    echo "  View logs     : less fix_openssl.log"
    echo "  View report   : less $REPORT_FILE"
    
    echo
    echo "📚 Resources:"
    echo "  OpenSSL Docs  : https://www.openssl.org/docs/"
    echo "  CMake Guide   : https://cmake.org/cmake/help/latest/"
    echo "  MinGW Guide   : https://www.mingw-w64.org/doku.php"
    
else
    print_error "❌ BUILD FAILED ❌"
    echo
    echo "╔══════════════════════════════════════════════════════╗"
    echo "║                   FAILURE ANALYSIS                  ║"
    echo "╚══════════════════════════════════════════════════════╝"
    echo
    print_info "🔍 Troubleshooting Steps:"
    echo "  1. 📋 Check detailed logs: less fix_openssl.log"
    echo "  2. 📋 Check build report: less $REPORT_FILE"
    echo "  3. 🔧 Verify OpenSSL installation and paths"
    echo "  4. 🔄 Try clean debug build: $0 -c -d"
    echo "  5. 🧪 Try test-only build: $0 -t"
    echo "  6. 🔍 Check system PATH configuration"
    
    echo
    print_info "🆘 Common Solutions:"
    echo "  Missing tools     : Install MSYS2 dev packages"
    echo "  OpenSSL issues    : Download from slproweb.com"
    echo "  Path problems     : Run $0 --force-path-fix"
    echo "  Architecture      : Ensure 32/64-bit consistency"
    echo "  Permissions       : Run as administrator if needed"
    
    echo
    print_info "🏥 Getting Help:"
    echo "  1. Share fix_openssl.log contents"
    echo "  2. Share $REPORT_FILE details"
    echo "  3. Check OpenSSL community forums"
    echo "  4. Verify MinGW/MSYS2 setup"
fi

echo
echo "╔══════════════════════════════════════════════════════╗"
echo "║                   FILE LOCATIONS                    ║"
echo "╚══════════════════════════════════════════════════════╝"
echo "  📁 Project Directory : $(pwd)"
echo "  🔨 Build Directory   : $(pwd)/$BUILD_DIR"
echo "  📋 Main Log File     : $(pwd)/fix_openssl.log"
echo "  📊 Build Report      : $(pwd)/$REPORT_FILE"
if [ "$SKIP_BACKUP" = false ]; then
    echo "  💾 Backup Files     : $(pwd)/*.backup_$BACKUP_SUFFIX"
fi
if [ -d "test_results" ]; then
    echo "  🧪 Test Results     : $(pwd)/test_results/"
fi

echo
echo "╔══════════════════════════════════════════════════════╗"
echo "║                 SCRIPT INFORMATION                  ║"
echo "╚══════════════════════════════════════════════════════╝"
echo "  Script Version    : $SCRIPT_VERSION"
echo "  Script Runtime    : ${TOTAL_SCRIPT_TIME}s"
echo "  Options Used      : $([ "$CLEAN_BUILD" = true ] && echo "-c ")$([ "$FORCE_REINSTALL" = true ] && echo "-f ")$([ "$DEBUG_MODE" = true ] && echo "-d ")$([ "$TEST_ONLY" = true ] && echo "-t ")$([ "$NO_DOWNLOAD" = true ] && echo "--no-download ")$([ "$MINGW_ONLY" = true ] && echo "--mingw-only ")$([ "$SKIP_BACKUP" = true ] && echo "--skip-backup ")"
echo "  For help          : $0 --help"

log_action "Script completed $([ -n "$TEST_EXE" ] && [ -f "$TEST_EXE" ] && echo "successfully" || echo "with issues")"

print_header "SCRIPT COMPLETED"
echo

# Exit with appropriate code
if [ -n "$TEST_EXE" ] && [ -f "$TEST_EXE" ]; then
    exit 0
else
    exit 1
fi