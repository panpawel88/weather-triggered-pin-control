# RTC Clock Set Test Application

Interactive utility to set the DS3231 RTC (Real-Time Clock) module time via serial interface.

## Purpose

This test application allows you to manually set the current date and time on the DS3231 RTC module. This is useful for:
- Initial RTC setup
- Correcting RTC time drift
- Testing RTC functionality
- Resetting time after battery replacement

## Hardware Requirements

- ESP32 development board (ESP32-S3 by default)
- DS3231 RTC module connected via I2C
- I2C pins configured in `hardware_config.h`:
  - **SDA: GPIO 2** (defined by `HW_I2C_SDA_PIN`)
  - **SCL: GPIO 1** (defined by `HW_I2C_SCL_PIN`)
- **Pull-up resistors**: 4.7kΩ - 10kΩ on both SDA and SCL lines (may be built into your DS3231 module)
- **DS3231 power**: 3.3V or 5V (VCC) and GND must be connected
- **Optional**: CR2032 battery in DS3231 for timekeeping when ESP32 is powered off

### Hardware Setup Checklist

Before running this application, verify:

1. ✅ DS3231 VCC connected to ESP32 3.3V or 5V
2. ✅ DS3231 GND connected to ESP32 GND
3. ✅ DS3231 SDA connected to ESP32 GPIO 2
4. ✅ DS3231 SCL connected to ESP32 GPIO 1
5. ✅ Pull-up resistors present (check your DS3231 module - many have built-in resistors)
6. ✅ Connections are secure and not loose

**IMPORTANT**: Run the `clock_test` application FIRST to verify your hardware is working before attempting to set the time!

## Building and Running

### Quick Start

**macOS/Linux:**
```bash
cd tools/test_apps
./build-test.sh clock_set flash-monitor
```

**Windows:**
```cmd
cd tools\test_apps
build-test.bat clock_set flash-monitor
```

### Manual Build

```bash
cd tools/test_apps/clock_set
idf.py build flash monitor
```

## Usage

### Interactive Input - IMPORTANT!

⚠️ **This application requires interactive keyboard input** and `idf.py monitor` **does NOT support this properly** on most platforms, especially Windows.

**You MUST use a dedicated serial terminal application** to set the RTC time:

#### Required Serial Terminals

**Windows:**
- **PuTTY** (recommended)
  1. Download from https://www.putty.org/
  2. Select "Serial" connection type
  3. Set COM port (e.g., COM3)
  4. Baud rate: 115200
  5. Click "Open"

- **Tera Term**
  1. Download from https://ttssh2.osdn.jp/
  2. Select Serial connection
  3. Set COM port and 115200 baud

**Linux/Mac:**
- **screen** (usually pre-installed)
  ```bash
  # Find device (usually /dev/ttyUSB0 on Linux, /dev/cu.usbserial-* on Mac)
  ls /dev/tty*

  # Connect
  screen /dev/ttyUSB0 115200

  # Exit: Ctrl+A then K, then Y
  ```

- **minicom**
  ```bash
  # Install if needed
  sudo apt-get install minicom  # Linux
  brew install minicom           # Mac

  # Connect
  minicom -D /dev/ttyUSB0 -b 115200

  # Exit: Ctrl+A then X
  ```

### Setting the Time

1. **Flash and monitor the application** using your preferred method
2. **Wait for the prompt**: The application will show current RTC time and prompt for input
3. **Enter date and time** when prompted:
   - Year (2000-2099)
   - Month (1-12)
   - Day (1-31)
   - Hour (0-23, 24-hour format)
   - Minute (0-59)
   - Second (0-59)
4. **Confirm**: Type `y` or `yes` to confirm setting the time
5. **Verification**: The application will read back the time to verify it was set correctly

### Example Session

```
========================================
  RTC Clock Set Application
========================================

Configuring UART for interactive input...
UART configured successfully

NOTE: If you cannot enter text, please use a serial terminal
      like PuTTY (Windows) or screen/minicom (Linux/Mac)
      instead of 'idf.py monitor' for better input support.

I2C Configuration:
  SDA Pin: 8
  SCL Pin: 9

Initializing I2C...
I2C initialized successfully

Reading current RTC time...
Current RTC time: 2024-01-15 14:30:45

========================================
Enter new date and time
========================================

Year (2000-2099): 2025
Month (1-12): 10
Day (1-31): 19
Hour (24h format) (0-23): 15
Minute (0-59): 45
Second (0-59): 0

========================================
New time to be set:
  2025-10-19 15:45:00
========================================

Confirm setting this time? (y/n): y
Writing time to RTC...
Time set successfully!

Verifying...
RTC now reads: 2025-10-19 15:45:00

Clock set operation complete!

Application will now exit
```

## Troubleshooting

### Cannot Enter Text

**Problem**: Typing has no effect, or you see "Error reading input" messages

**Solutions**:
1. Use a dedicated serial terminal (see "Recommended Serial Terminals" above)
2. On Windows, ensure the correct COM port is selected
3. Try disconnecting and reconnecting the USB cable
4. Verify baud rate is set to 115200

### I2C Initialization Failed

**Problem**: Application shows "I2C initialization failed!"

**Solutions**:
1. Check RTC module is properly connected
2. Verify SDA/SCL pin configuration in `hardware_config.h`
3. Check I2C pull-up resistors (usually 4.7kΩ to 10kΩ)
4. Verify RTC module power supply (3.3V or 5V depending on module)

### Invalid Time After Setting

**Problem**: Time verification shows incorrect values

**Solutions**:
1. Ensure date/time values are within valid ranges
2. Check RTC battery is installed and charged
3. Verify I2C communication is stable
4. Try setting the time again

### RTC Loses Time

**Problem**: RTC does not keep time when ESP32 is powered off

**Solutions**:
1. Install a CR2032 battery in the RTC module
2. Replace the battery if it's old
3. Check battery holder contacts

## Configuration

Hardware configuration is inherited from the main project via `hardware_config.h`. To change I2C pins or other settings, edit:

```
components/weather_common/include/hardware_config.h
```

Then rebuild the application.

## See Also

- **clock_test** - Read and display current RTC time
- **config_print** - Display all hardware configuration
- Main project documentation in `CLAUDE.md`
