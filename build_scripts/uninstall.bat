@echo off
setlocal enabledelayedexpansion

echo ========================================
echo    Keylogger Research Tool Uninstaller
echo ========================================
echo.

set INSTALL_DIR=%ProgramFiles%\KeyloggerResearch
set TOKEN_FILE=%INSTALL_DIR%\.uninstall_token

if not exist "%INSTALL_DIR%" (
    echo Tool is not installed.
    pause
    exit /b 1
)

if not exist "%TOKEN_FILE%" (
    echo Token file missing. Uninstallation aborted.
    echo Please contact administrator for assistance.
    pause
    exit /b 1
)

set /p "STORED_TOKEN=<%TOKEN_FILE%"
set /p "INPUT_TOKEN=Enter uninstall token: "

if "!INPUT_TOKEN!" neq "!STORED_TOKEN!" (
    echo Invalid token. Uninstallation aborted.
    pause
    exit /b 1
)

:: Remove installation directory
echo Removing installation directory...
rmdir /s /q "%INSTALL_DIR%"

:: Remove from PATH (optional)
set "NEW_PATH=%PATH%"
set "NEW_PATH=!NEW_PATH:;%INSTALL_DIR%\build\Release=!"
setx PATH "!NEW_PATH!" /M >nul

echo.
echo ========================================
echo Tool uninstalled successfully!
echo ========================================
echo.

pause
endlocal