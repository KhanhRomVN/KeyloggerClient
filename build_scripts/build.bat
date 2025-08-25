@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=%~dp0..\build
set DIST_DIR=%~dp0..\dist
set SRC_DIR=%~dp0..
set TOOLS_DIR=%~dp0..\tools
set CMAKE_VERSION=3.25.2
set CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v%CMAKE_VERSION%/cmake-%CMAKE_VERSION%-windows-x86_64.msi
set CMAKE_INSTALLER=%TOOLS_DIR%\cmake-%CMAKE_VERSION%-windows-x86_64.msi

echo [INFO] Checking and creating directories...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
if not exist "%TOOLS_DIR%" mkdir "%TOOLS_DIR%"

echo [INFO] Checking CMake installation...
where cmake >nul 2>nul
if %errorlevel% equ 0 (
    echo [INFO] CMake found in PATH.
    goto :build
)

echo [INFO] CMake not found in PATH. Checking local installation...
if exist "%TOOLS_DIR%\cmake-%CMAKE_VERSION%-windows-x86_64\bin\cmake.exe" (
    echo [INFO] Local CMake found. Adding to PATH...
    set "PATH=%TOOLS_DIR%\cmake-%CMAKE_VERSION%-windows-x86_64\bin;%PATH%"
    goto :build
)

echo [INFO] Downloading CMake %CMAKE_VERSION%...
powershell -Command "Invoke-WebRequest -Uri '%CMAKE_URL%' -OutFile '%CMAKE_INSTALLER%'"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to download CMake
    exit /b 1
)

echo [INFO] Installing CMake...
msiexec /i "%CMAKE_INSTALLER%" /qn /norestart TARGETDIR="%TOOLS_DIR%\cmake-%CMAKE_VERSION%-windows-x86_64"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to install CMake
    exit /b 1
)

echo [INFO] Adding CMake to PATH...
set "PATH=%TOOLS_DIR%\cmake-%CMAKE_VERSION%-windows-x86_64\bin;%PATH%"

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