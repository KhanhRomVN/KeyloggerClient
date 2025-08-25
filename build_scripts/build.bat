@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=%~dp0..\build
set DIST_DIR=%~dp0..\dist
set SRC_DIR=%~dp0..
set CMAKE_VERSION=3.25.2

echo [INFO] Checking and creating directories...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"

echo [INFO] Checking CMake installation...
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found in PATH. Please install CMake and add it to your system PATH.
    echo [INFO] You can download CMake from: https://cmake.org/download/
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