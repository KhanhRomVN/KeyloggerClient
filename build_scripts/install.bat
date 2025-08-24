@echo off
setlocal enabledelayedexpansion

echo ========================================
echo    Keylogger Research Tool Installer
echo ========================================
echo.

:: Set paths and URLs
set INSTALL_DIR=%ProgramFiles%\KeyloggerResearch
set REPO_URL=https://github.com/KhanhRomVN/KeyloggerResearch
set SCRIPTS_DIR=%~dp0
set TOKEN_FILE=%INSTALL_DIR%\.uninstall_token

:: Create installation directory
if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%"
    echo Created installation directory: %INSTALL_DIR%
) else (
    echo Installation directory already exists: %INSTALL_DIR%
)

:: Clone or download the repository
echo Downloading Keylogger Research Tool from GitHub...
if exist "%INSTALL_DIR%\.git" (
    cd "%INSTALL_DIR%"
    git pull origin main
) else (
    git clone %REPO_URL% "%INSTALL_DIR%"
)

if %errorlevel% neq 0 (
    echo Failed to download the tool from GitHub.
    pause
    exit /b 1
)

:: Build the project
echo Building the project...
cd "%INSTALL_DIR%"
if exist build.bat (
    call build.bat
) else (
    echo Build script not found. Trying to build with CMake...
    if not exist build mkdir build
    cd build
    cmake -G "Visual Studio 16 2019" -A x64 ..
    cmake --build . --config Release
)

if %errorlevel% neq 0 (
    echo Build failed.
    pause
    exit /b 1
)

:: Generate random token (8 characters)
set "CHARS=ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
set "TOKEN="
for /L %%i in (1,1,8) do (
    set /a "RAND=!RANDOM! %% 36"
    for %%j in (!RAND!) do set "TOKEN=!TOKEN!!CHARS:~%%j,1!"
)

:: Save token to hidden file
echo !TOKEN! > "%TOKEN_FILE%"
attrib +h "%TOKEN_FILE%"

:: Copy uninstall script to installation directory
copy "%SCRIPTS_DIR%\uninstall.bat" "%INSTALL_DIR%\uninstall.bat" >nul

:: Add to PATH (optional)
setx PATH "%PATH%;%INSTALL_DIR%\build\Release" /M >nul

echo.
echo ========================================
echo Installation completed successfully!
echo.
echo IMPORTANT: Save this uninstall token:
echo !TOKEN!
echo.
echo You will need it to uninstall the tool.
echo ========================================
echo.

pause
endlocal