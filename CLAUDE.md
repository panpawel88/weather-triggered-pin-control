# Weather Triggered Pin Control - Build Instructions

## Prerequisites

Before building this ESP32 project, ensure you have ESP-IDF installed:

- **ESP-IDF Installation**: Follow the official [ESP-IDF installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- **ESP32 Development Board**: Compatible ESP32 development board
- **USB Cable**: For flashing and monitoring

## Configuration

This project uses a **hybrid configuration system**:
- **config.h** (gitignored) - WiFi credentials and location overrides
- **menuconfig** - Hardware pins, weather settings, cloud cover ranges

### Step 1: Create Configuration File (Required)

```bash
# Copy the example config file
cp main/config.h.example main/config.h

# Edit config.h with your WiFi credentials
# IMPORTANT: config.h is gitignored and will not be committed
```

### Step 2: Customize Hardware Settings (Optional)

```bash
# Run menuconfig to configure pins and weather settings
idf.py menuconfig

# Navigate to: Weather Control Configuration
# - Hardware Pin Configuration (GPIO pins)
# - Location Settings (default lat/long)
# - Weather Check Schedule
# - Cloud Cover Ranges Configuration
```

## Build Instructions

### macOS/Linux

1. **Using the build script (recommended)**:
   ```bash
   ./build-macos.sh
   ```

2. **Manual build process**:
   ```bash
   # Set up environment (run every new terminal session)
   . $HOME/esp/esp-idf/export.sh

   # Set target
   idf.py set-target esp32

   # Build
   idf.py build
   ```

### Windows

1. **Using the build script (recommended)**:
   ```cmd
   build-windows.bat
   ```

2. **Manual build process**:
   ```cmd
   # Set up environment (run every new terminal session)
   %userprofile%\esp\esp-idf\export.bat

   # Set target
   idf.py set-target esp32

   # Build
   idf.py build
   ```

## Flashing and Monitoring

After a successful build:

1. **Flash the firmware**:
   ```bash
   idf.py flash
   ```

2. **Monitor serial output**:
   ```bash
   idf.py monitor
   ```

3. **Flash and monitor in one command**:
   ```bash
   idf.py flash monitor
   ```

## Project Configuration

- WiFi credentials are configured in `main/config.h` (must be created from config.h.example)
- Hardware pins and weather settings are configured via `idf.py menuconfig`
- The target is set to ESP32 by default (ESP32-S3 also supported)
- Build artifacts are generated in the `build/` directory
- Configuration values are stored in `sdkconfig` (auto-generated)

## Troubleshooting

- Ensure ESP-IDF is properly installed and sourced
- Check that your ESP32 board is connected and recognized
- Verify the correct COM port is selected for flashing
- Run `idf.py clean` if you encounter build issues