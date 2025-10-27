# Weather Triggered Pin Control

> **⚠️ DEVELOPMENT STATUS**: This project is currently in development and has not been fully tested. Use at your own risk.

An ESP32-based IoT device that automatically controls a GPIO pin based on weather forecasts. The device fetches weather data from Open-Meteo API and activates a control pin when sunny weather is predicted for the next day.

## Features

- **Weather Forecast Integration**: Fetches weather data from Open-Meteo API
- **Smart Scheduling**: Checks weather at 4 PM daily and controls pin the next morning if sunny
- **Real-Time Clock Support**: Uses DS3231 RTC module for accurate timekeeping
- **Low Power Design**: Utilizes ESP32 deep sleep mode to conserve battery
- **WiFi Connectivity**: Connects to WiFi for weather data retrieval
- **Configurable Location**: Easy to configure for any geographic location

## Hardware Requirements

- ESP32 development board
- DS3231 Real-Time Clock module
- Connecting wires
- Optional: External components to control (relay, LED, etc.)

## Default Pin Configuration

Default pins (configurable in `hardware_config.h`):

- **GPIO 13**: Control pin output
- **GPIO 5, 6, 7, 15, 16**: LED pins (cloud cover visualization)
- **GPIO 1**: I2C SDA (DS3231)
- **GPIO 2**: I2C SCL (DS3231)
- **GPIO 48**: RGB LED (optional status indicator)

**Note:** All pins can be customized by editing `components/hardware_config/include/hardware_config.h`

## Software Prerequisites

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) (Espressif IoT Development Framework)
- Compatible ESP32 development board
- USB cable for flashing and monitoring

## Configuration

This project uses a simple configuration system based on two header files.

### Step 1: Create WiFi Configuration File

Copy the example config file and edit it with your WiFi credentials:

```bash
# Create config.h from template
cp components/hardware_config/include/config.h.example components/hardware_config/include/config.h

# Edit config.h with your WiFi credentials and optional location
# (Use your preferred text editor)
```

**In `components/hardware_config/include/config.h`, configure:**
- `WIFI_SSID` - Your WiFi network name
- `WIFI_PASSWORD` - Your WiFi password
- `LATITUDE` / `LONGITUDE` - (Optional) Override default location
- `REMOTE_LOG_SERVER_URL` - (Optional) Remote logging server URL

**Note:** `config.h` is gitignored and will not be committed to the repository.

### Step 2: Configure Hardware and Behavior (Optional)

All hardware pins and behavior settings are configured in:
- `components/hardware_config/include/hardware_config.h`

**Available settings:**
- GPIO pin assignments (control pin, LEDs, I2C)
- Cloud cover ranges and pin-off hours
- Weather check schedule (default: 4 PM)
- Default location (latitude/longitude)
- Remote logging settings
- RGB LED configuration

Edit this file directly and rebuild the project to apply changes.

## Building and Flashing

1. **Set up ESP-IDF environment:**
   ```bash
   # macOS/Linux
   . $HOME/esp/esp-idf/export.sh

   # Windows
   %userprofile%\esp\esp-idf\export.bat
   ```

2. **Build the project:**
   ```bash
   idf.py set-target esp32
   idf.py build
   ```

3. **Flash and monitor:**
   ```bash
   idf.py flash monitor
   ```

## How It Works

1. **Daily Weather Check**: At 4 PM each day, the ESP32 wakes up and connects to WiFi
2. **API Request**: Fetches tomorrow's weather forecast from Open-Meteo API
3. **Decision Logic**: Determines if tomorrow will be sunny based on weather data
4. **Deep Sleep**: Goes back to sleep until the next check or control time
5. **Morning Control**: If sunny weather was predicted, activates the control pin at 8 AM
6. **Repeat Cycle**: Returns to deep sleep and repeats the process

## Weather API

This project uses the [Open-Meteo API](https://open-meteo.com/) which provides:
- Free weather forecasts
- No API key required
- Global coverage
- Reliable service

## Power Management

The device is designed for low power consumption:
- Deep sleep mode between operations
- RTC module maintains time during sleep
- Weather data stored in RTC memory
- Typical power consumption: ~10µA in sleep mode

## Troubleshooting

- **Build Issues**: Run `idf.py clean` and rebuild
- **WiFi Connection**: Verify SSID and password in configuration
- **Time Issues**: Check DS3231 RTC module connections
- **Weather Data**: Ensure internet connectivity and valid coordinates

## License

This project is open source. See the project files for more details.

## Contributing

Feel free to submit issues and enhancement requests!