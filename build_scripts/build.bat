@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=%~dp0..\build
set DIST_DIR=%~dp0..\dist
set SRC_DIR=%~dp0..
set CMAKE_VERSION=3.25.2
set CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v%CMAKE_VERSION%/cmake-%CMAKE_VERSION%-windows-x86_64.msi
set TEMP_DOWNLOAD=%~dp0cmake_installer.msi

echo [INFO] Checking and creating directories...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"

echo [INFO] Checking CMake installation...
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [WARN] CMake not found in PATH. Attempting to download and install...
    
    REM Download CMake
    echo [INFO] Downloading CMake v%CMAKE_VERSION%...
    powershell -Command "Invoke-WebRequest -Uri '%CMAKE_URL%' -OutFile '%TEMP_DOWNLOAD%'"
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to download CMake
        exit /b 1
    )
    
    REM Install CMake silently
    echo [INFO] Installing CMake...
    msiexec /i "%TEMP_DOWNLOAD%" /quiet /norestart ADD_CMAKE_TO_PATH=System
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to install CMake
        exit /b 1
    )
    
    REM Clean up installer
    del "%TEMP_DOWNLOAD%" 2>nul
    
    REM Wait for installation to complete and refresh PATH
    timeout /t 5 /nobreak >nul
    echo [INFO] Refreshing PATH environment...
    set PATH=%PATH%;C:\Program Files\CMake\bin
)

echo [INFO] Verifying CMake installation...
cmake --version
if %errorlevel% neq 0 (
    echo [ERROR] CMake installation verification failed
    exit /b 1
)

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