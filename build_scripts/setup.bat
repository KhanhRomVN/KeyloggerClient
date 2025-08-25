@echo off
echo [INFO] Setting up build environment...
echo [INFO] Creating necessary directories...

mkdir build 2>nul
mkdir dist 2>nul
mkdir logs 2>nul

echo [INFO] Directory structure created:
tree /f

echo [INFO] Setup completed. Run build.bat to build the project.
pause