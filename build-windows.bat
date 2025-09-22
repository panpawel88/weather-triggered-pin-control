@echo off
REM ESP32 Build Script for Windows
REM This script sets up the ESP-IDF environment and builds the project

echo Setting up ESP-IDF environment...
REM Set up environment
call %userprofile%\esp\esp-idf\export.bat

echo Configuring project for ESP32...
REM Configure project
idf.py set-target esp32

echo Opening menuconfig (optional - press Ctrl+C to skip)...
REM Optional: Open menuconfig for configuration
REM Uncomment the next line if you want to configure the project
REM idf.py menuconfig

echo Building project...
REM Build the project
idf.py build

if %ERRORLEVEL% EQU 0 (
    echo Build completed successfully!
    echo To flash the project, run: idf.py flash
    echo To monitor serial output, run: idf.py monitor
) else (
    echo Build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)