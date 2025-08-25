@echo off
setlocal enabledelayedexpansion

:: ================================
:: Config
:: ================================
set BUILD_DIR=%~dp0..\build
set DIST_DIR=%~dp0..\dist
set SRC_DIR=%~dp0..
set TOOLS_DIR=%~dp0..\tools
set CMAKE_VERSION=3.25.2
set CMAKE_ZIP=cmake-%CMAKE_VERSION%-windows-x86_64.zip
set CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v%CMAKE_VERSION%/%CMAKE_ZIP%
set CMAKE_DIR=%TOOLS_DIR%\cmake-%CMAKE_VERSION%-windows-x86_64

:: ================================
:: Tạo thư mục cần thiết
:: ================================
echo [INFO] Checking and creating directories...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
if not exist "%TOOLS_DIR%" mkdir "%TOOLS_DIR%"

:: ================================
:: Kiểm tra CMake
:: ================================
echo [INFO] Checking CMake installation...
where cmake >nul 2>nul
if %errorlevel% equ 0 (
    echo [INFO] CMake found in PATH.
    goto :build
)

echo [INFO] CMake not found in PATH. Checking local copy...
if exist "%CMAKE_DIR%\bin\cmake.exe" (
    echo [INFO] Local CMake found. Adding to PATH...
    set "PATH=%CMAKE_DIR%\bin;%PATH%"
    goto :build
)

:: ================================
:: Download & Extract CMake zip
:: ================================
echo [INFO] Downloading CMake %CMAKE_VERSION%...
powershell -Command "Invoke-WebRequest '%CMAKE_URL%' -OutFile '%TOOLS_DIR%\%CMAKE_ZIP%'"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to download CMake
    echo [INFO] Please download manually: https://cmake.org/download/
    exit /b 1
)

echo [INFO] Extracting CMake...
powershell -Command "Expand-Archive '%TOOLS_DIR%\%CMAKE_ZIP%' -DestinationPath '%TOOLS_DIR%' -Force"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to extract CMake
    exit /b 1
)

set "PATH=%CMAKE_DIR%\bin;%PATH%"

:: ================================
:: Build Project
:: ================================
:build
echo [INFO] Configuring build...
cd "%BUILD_DIR%"
cmake -G "Visual Studio 16 2019" -A x64 ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%DIST_DIR%" ^
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="%DIST_DIR%" ^
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="%DIST_DIR%" ^
      "%SRC_DIR%"

if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed
    exit /b 1
)

echo [INFO] Building project...
cmake --build . --config Release --target ALL_BUILD -j 8

if %errorlevel% neq 0 (
    echo [ERROR] Build failed
    exit /b 1
)

echo [SUCCESS] Build completed! Executable is in: %DIST_DIR%
dir "%DIST_DIR%"

endlocal
