# Weather Triggered Pin Control - Build Instructions

## Prerequisites

Before building this ESP32 project, ensure you have ESP-IDF installed:

- **ESP-IDF Installation**: Follow the official [ESP-IDF installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- **ESP32 Development Board**: Compatible ESP32 development board
- **USB Cable**: For flashing and monitoring

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

   # Configure project
   idf.py set-target esp32
   idf.py menuconfig  # Optional: configure project settings

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

   # Configure project
   idf.py set-target esp32
   idf.py menuconfig  # Optional: configure project settings

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

- Run `idf.py menuconfig` to configure project-specific settings
- The target is set to ESP32 by default
- Build artifacts are generated in the `build/` directory

## Troubleshooting

- Ensure ESP-IDF is properly installed and sourced
- Check that your ESP32 board is connected and recognized
- Verify the correct COM port is selected for flashing
- Run `idf.py clean` if you encounter build issues