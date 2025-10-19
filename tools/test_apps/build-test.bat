@echo off
REM Build script for test applications (Windows)
REM Usage: build-test.bat <test_name> [command]
REM Where test_name is one of: led_test, weather_test, clock_test, clock_set, config_print, rgb_led_test

setlocal enabledelayedexpansion

REM Check if ESP-IDF is sourced
if "%IDF_PATH%"=="" (
    echo Error: ESP-IDF environment not set up
    echo Please run: %%userprofile%%\esp\esp-idf\export.bat
    exit /b 1
)

REM Check arguments
if "%1"=="" (
    echo Usage: %0 ^<test_name^> [build^|flash^|monitor^|flash-monitor^|clean]
    echo.
    echo Available tests:
    echo   led_test      - LED sequential flash test
    echo   weather_test  - Weather forecast fetch test
    echo   clock_test    - RTC clock read test
    echo   clock_set     - RTC clock set utility
    echo   config_print  - Print all configuration
    echo   rgb_led_test  - ESP32-S3 RGB LED rainbow test
    echo.
    echo Available commands ^(default: build^):
    echo   build         - Build the test application
    echo   flash         - Flash the test application
    echo   monitor       - Open serial monitor
    echo   flash-monitor - Flash and monitor
    echo   clean         - Clean build artifacts
    echo.
    echo Examples:
    echo   %0 led_test build
    echo   %0 weather_test flash-monitor
    echo   %0 clock_set clean
    exit /b 1
)

set TEST_NAME=%1
if "%2"=="" (
    set COMMAND=build
) else (
    set COMMAND=%2
)

REM Validate test name
set VALID=0
if "%TEST_NAME%"=="led_test" set VALID=1
if "%TEST_NAME%"=="weather_test" set VALID=1
if "%TEST_NAME%"=="clock_test" set VALID=1
if "%TEST_NAME%"=="clock_set" set VALID=1
if "%TEST_NAME%"=="config_print" set VALID=1
if "%TEST_NAME%"=="rgb_led_test" set VALID=1

if %VALID%==0 (
    echo Error: Invalid test name '%TEST_NAME%'
    echo Valid tests: led_test, weather_test, clock_test, clock_set, config_print, rgb_led_test
    exit /b 1
)

set TEST_DIR=%TEST_NAME%

REM Change to test directory
cd "%TEST_DIR%"

echo =========================================
echo Test Application: %TEST_NAME%
echo Command: %COMMAND%
echo Directory: %CD%
echo =========================================
echo.

REM Execute command
if "%COMMAND%"=="build" (
    idf.py build
) else if "%COMMAND%"=="flash" (
    idf.py flash
) else if "%COMMAND%"=="monitor" (
    idf.py monitor
) else if "%COMMAND%"=="flash-monitor" (
    idf.py flash monitor
) else if "%COMMAND%"=="clean" (
    idf.py fullclean
) else (
    echo Error: Unknown command '%COMMAND%'
    echo Valid commands: build, flash, monitor, flash-monitor, clean
    exit /b 1
)

echo.
echo =========================================
echo Done!
echo =========================================

endlocal
