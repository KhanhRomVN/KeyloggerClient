@echo off
setlocal
set CERT_FILE=%~dp0..\resources\certificates\dummy_cert.pem
set EXE_FILE=%~dp0..\build\bin\KeyloggerClientProject.exe

if not exist "%EXE_FILE%" (
    echo Error: Executable not found
    exit /b 1
)

REM This is a placeholder for actual signing process
echo Simulating code signing with %CERT_FILE%
echo File would be signed: %EXE_FILE%
echo Actual signing would require a valid code signing certificate

endlocal