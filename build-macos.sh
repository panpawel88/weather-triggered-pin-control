#!/bin/bash

# ESP32 Build Script for macOS/Linux
# This script sets up the ESP-IDF environment and builds the project

set -e  # Exit on any error

echo "Setting up ESP-IDF environment..."
# Set up environment
. $HOME/esp/esp-idf/export.sh

echo "Configuring project for ESP32..."
# Configure project
idf.py set-target esp32

echo "Opening menuconfig (optional - press Ctrl+C to skip)..."
# Optional: Open menuconfig for configuration
# Uncomment the next line if you want to configure the project
# idf.py menuconfig

echo "Building project..."
# Build the project
idf.py build

echo "Build completed successfully!"
echo "To flash the project, run: idf.py flash"
echo "To monitor serial output, run: idf.py monitor"