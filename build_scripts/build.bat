@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=%~dp0..\build
set SRC_DIR=%~dp0..
set GENERATOR="Visual Studio 16 2019"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

cmake -G %GENERATOR% -A x64 -DCMAKE_BUILD_TYPE=Release "%SRC_DIR%"
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build . --config Release --target ALL_BUILD -j 8
if %errorlevel% neq 0 exit /b %errorlevel%

echo Build completed successfully!
endlocal