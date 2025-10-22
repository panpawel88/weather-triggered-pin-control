# Weather Triggered Pin Control - Build Instructions

## Prerequisites

Before building this ESP32 project, ensure you have ESP-IDF installed:

- **ESP-IDF Installation**: Follow the official [ESP-IDF installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- **ESP32 Development Board**: Compatible ESP32 development board
- **USB Cable**: For flashing and monitoring

## Configuration

This project uses a **simple, single-file configuration system**:

### Single Source of Truth: `hardware_config.h`

**All hardware and behavior configuration** is in one file:
- `components/weather_common/include/hardware_config.h`

This file contains:
- ✅ GPIO pin assignments (control pin, LEDs, I2C)
- ✅ Cloud cover ranges and pin-off hours
- ✅ Weather check schedule
- ✅ Default location (lat/long)

**To change your configuration:** Simply edit `hardware_config.h` and rebuild.

### WiFi Credentials: `config.h` (gitignored)

WiFi credentials are kept separate for security:

```bash
# Copy the example config file
cp components/weather_common/include/config.h.example components/weather_common/include/config.h

# Edit config.h with your WiFi credentials and optional location override
# IMPORTANT: config.h is gitignored and will not be committed
```

**No menuconfig needed!** Everything is configured through simple header files.

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

## Remote Logging

The ESP32 can send logs to a remote HTTP server for centralized monitoring when disconnected from your PC. This is useful for production deployments where the device runs on an AC adapter without a serial connection.

### Features

- **Automatic buffering**: Logs are buffered in memory (up to 100 messages by default)
- **FIFO overflow**: When buffer is full, oldest messages are dropped
- **Timestamp tracking**: Each log includes RTC timestamp
- **WiFi efficient**: Logs are sent during weather fetch (when WiFi is already active)
- **Graceful degradation**: If server is unreachable, logs are retained and retried

### Configuration

**1. Enable remote logging in `hardware_config.h`:**

```c
#define HW_REMOTE_LOGGING_ENABLED true    // Enable/disable remote logging
#define HW_LOG_BUFFER_SIZE 100            // Max number of log messages
#define HW_LOG_DEVICE_NAME "weather-esp32" // Device identifier
```

**2. Configure server URL in `config.h`:**

```c
#define REMOTE_LOG_SERVER_URL "http://192.168.1.100:3000/api/logs"
```

Replace `192.168.1.100` with your server's IP address.

### Setting Up the Log Server

Two ready-to-use server implementations are provided in `server_examples/`:

#### Option 1: Node.js Server

```bash
cd server_examples/nodejs
npm install
npm start
```

#### Option 2: Python Server

```bash
cd server_examples/python
pip install -r requirements.txt
python log_server.py
```

Both servers:
- Listen on port 3000 by default (configurable)
- Save logs to `device_logs/` directory
- Organize logs by device name and date
- Support verbose mode to print logs to console

See `server_examples/nodejs/README.md` or `server_examples/python/README.md` for detailed setup instructions.

### How It Works

1. ESP32 buffers all log messages during operation
2. When WiFi connects for weather fetch (4 PM daily), buffered logs are sent to server
3. Server saves logs with timestamps for later analysis
4. If server is unreachable, logs stay in buffer and retry on next connection
5. If buffer fills up, oldest messages are dropped (FIFO)

### Finding Your Server IP Address

**Windows:**
```cmd
ipconfig
```
Look for "IPv4 Address" (e.g., 192.168.1.100)

**Mac/Linux:**
```bash
ifconfig
# or
ip addr
```
Look for "inet" address on your active network interface.

### Disabling Remote Logging

Set `HW_REMOTE_LOGGING_ENABLED false` in `hardware_config.h` to disable remote logging entirely. Serial logging will continue to work normally.

## Project Configuration

- WiFi credentials are configured in `main/config.h` (must be created from config.h.example)
- Hardware pins and weather settings are configured via `idf.py menuconfig`
- The target is set to ESP32 by default (ESP32-S3 also supported)
- Build artifacts are generated in the `build/` directory
- Configuration values are stored in `sdkconfig` (auto-generated)

## Test Applications

This project includes separate test applications for hardware validation and debugging, located in `tools/test_apps/`. These applications are completely separate from the production code and use shared components from `components/weather_common/`.

### Available Test Applications

1. **led_test** - LED Sequential Flash Test
   - Flashes LEDs one after another in sequence (1→2→3→4→5, repeat)
   - Useful for verifying LED wiring and active-low logic
   - Runs continuously until interrupted

2. **weather_test** - Weather Forecast Fetch Test
   - Connects to WiFi and fetches weather forecast from Open-Meteo API
   - Displays tomorrow's cloud cover percentage
   - Shows corresponding LED count and weather condition

3. **clock_test** - RTC Clock Read Test
   - Reads current time from DS3231 RTC
   - Displays time every 2 seconds in multiple formats
   - Useful for verifying I2C communication and RTC operation

4. **clock_set** - RTC Clock Set Utility
   - Interactive serial menu to set the DS3231 RTC time
   - Prompts for year, month, day, hour, minute, second
   - Verifies the time was set correctly
   - **NOTE**: Requires interactive input - see special instructions below

5. **config_print** - Configuration Print Utility
   - Prints all hardware pin assignments
   - Shows WiFi SSID (password masked)
   - Displays location, weather schedule, and cloud cover ranges

### Building and Running Test Applications

#### Using the build script (recommended):

**macOS/Linux:**
```bash
cd tools/test_apps
./build-test.sh <test_name> [command]
```

**Windows:**
```cmd
cd tools\test_apps
build-test.bat <test_name> [command]
```

**Available commands:**
- `build` - Build the test application (default)
- `flash` - Flash the test application
- `monitor` - Open serial monitor
- `flash-monitor` - Flash and monitor in one command
- `clean` - Clean build artifacts

**Examples:**
```bash
# Build LED test
./build-test.sh led_test build

# Flash and monitor weather test
./build-test.sh weather_test flash-monitor

# Clean clock set application
./build-test.sh clock_set clean
```

#### Manual build process:

```bash
# Navigate to specific test application
cd tools/test_apps/led_test

# Set up ESP-IDF environment (if not already done)
. $HOME/esp/esp-idf/export.sh

# Build, flash, and monitor
idf.py build flash monitor
```

### Test Application Notes

- All test apps inherit hardware pin configuration from menuconfig
- WiFi credentials come from `main/config.h` (for weather_test)
- Each test app can be configured independently via `idf.py menuconfig` in its directory
- Test applications are set to target ESP32-S3 by default (can be changed)
- No deep sleep - test apps run continuously or exit after completion

#### Interactive Input Applications (clock_set)

The **clock_set** application requires interactive keyboard input to set the RTC time. Due to limitations with `idf.py monitor` on some platforms (especially Windows), you may not be able to enter text.

**If you cannot enter text with `idf.py monitor`**, use a dedicated serial terminal instead:

**Windows:**
- **PuTTY** (recommended): Download from https://www.putty.org/
  - Connection type: Serial
  - COM port: (find with Device Manager, usually COM3)
  - Baud rate: 115200
- **Tera Term**: Alternative serial terminal

**Linux/Mac:**
- **screen**: `screen /dev/ttyUSB0 115200` (exit with Ctrl+A then K)
- **minicom**: `minicom -D /dev/ttyUSB0 -b 115200` (exit with Ctrl+A then X)

**Workflow:**
1. Flash the application: `./build-test.sh clock_set flash` (or `build-test.bat clock_set flash` on Windows)
2. Close the monitor if it auto-started
3. Connect using your preferred serial terminal
4. Follow the on-screen prompts to set the date and time

See `tools/test_apps/clock_set/README.md` for detailed usage instructions.

## Troubleshooting

- Ensure ESP-IDF is properly installed and sourced
- Check that your ESP32 board is connected and recognized
- Verify the correct COM port is selected for flashing
- Run `idf.py clean` if you encounter build issues