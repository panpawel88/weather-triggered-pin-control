#!/bin/bash

# Build script for test applications
# Usage: ./build-test.sh <test_name>
# Where test_name is one of: led_test, weather_test, clock_test, clock_set, config_print, rgb_led_test

set -e

# Check if ESP-IDF is sourced
if [ -z "$IDF_PATH" ]; then
    echo "Error: ESP-IDF environment not set up"
    echo "Please run: . \$HOME/esp/esp-idf/export.sh"
    exit 1
fi

# Check arguments
if [ $# -eq 0 ]; then
    echo "Usage: $0 <test_name> [build|flash|monitor|flash-monitor|clean]"
    echo ""
    echo "Available tests:"
    echo "  led_test      - LED sequential flash test"
    echo "  weather_test  - Weather forecast fetch test"
    echo "  clock_test    - RTC clock read test"
    echo "  clock_set     - RTC clock set utility"
    echo "  config_print  - Print all configuration"
    echo "  rgb_led_test  - ESP32-S3 RGB LED rainbow test"
    echo ""
    echo "Available commands (default: build):"
    echo "  build         - Build the test application"
    echo "  flash         - Flash the test application"
    echo "  monitor       - Open serial monitor"
    echo "  flash-monitor - Flash and monitor"
    echo "  clean         - Clean build artifacts"
    echo ""
    echo "Examples:"
    echo "  $0 led_test build"
    echo "  $0 weather_test flash-monitor"
    echo "  $0 clock_set clean"
    exit 1
fi

TEST_NAME=$1
COMMAND=${2:-build}

# Validate test name
VALID_TESTS="led_test weather_test clock_test clock_set config_print rgb_led_test"
if [[ ! " $VALID_TESTS " =~ " $TEST_NAME " ]]; then
    echo "Error: Invalid test name '$TEST_NAME'"
    echo "Valid tests: $VALID_TESTS"
    exit 1
fi

TEST_DIR="$TEST_NAME"

# Change to test directory
cd "$TEST_DIR"

echo "========================================="
echo "Test Application: $TEST_NAME"
echo "Command: $COMMAND"
echo "Directory: $(pwd)"
echo "========================================="
echo ""

# Execute command
case $COMMAND in
    build)
        idf.py build
        ;;
    flash)
        idf.py flash
        ;;
    monitor)
        idf.py monitor
        ;;
    flash-monitor)
        idf.py flash monitor
        ;;
    clean)
        idf.py fullclean
        ;;
    *)
        echo "Error: Unknown command '$COMMAND'"
        echo "Valid commands: build, flash, monitor, flash-monitor, clean"
        exit 1
        ;;
esac

echo ""
echo "========================================="
echo "Done!"
echo "========================================="
