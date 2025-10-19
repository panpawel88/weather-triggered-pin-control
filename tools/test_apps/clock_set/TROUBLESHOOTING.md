# Clock Set Application - Troubleshooting Guide

## Current Status

Based on your logs, there are **two issues** preventing the clock_set application from working:

### 1. No I2C Devices Found (HARDWARE ISSUE)

```
W (1118) CLOCK_SET: No I2C devices found! Check wiring and pull-up resistors.
E (1138) RTC_HELPER: Failed to read control register: ESP_FAIL
E (1168) RTC_HELPER: Failed to read from DS3231: ESP_FAIL
```

**Problem**: The ESP32 cannot detect ANY devices on the I2C bus, including the DS3231 RTC at address 0x68.

**Root Causes**:
- DS3231 is not powered (VCC/GND not connected)
- Wrong GPIO pins used for I2C connections
- Loose or broken wires
- Missing or inadequate pull-up resistors
- Faulty DS3231 module

**Solution**: Follow the hardware checklist below.

### 2. Interactive Input Not Working (SOFTWARE LIMITATION)

```
Year (2000-2099):
Error reading input. Please try again.
```

**Problem**: `idf.py monitor` does not properly support interactive stdin on Windows.

**Solution**: You MUST use PuTTY or another dedicated serial terminal (see instructions below).

---

## Hardware Setup Checklist

Your configuration (from `hardware_config.h`):
- **SDA Pin**: GPIO 2
- **SCL Pin**: GPIO 1

### Step-by-Step Hardware Verification

1. **Power Connections**
   - [ ] DS3231 VCC connected to ESP32 **3.3V** (recommended) or 5V
   - [ ] DS3231 GND connected to ESP32 **GND**
   - [ ] Connections are secure (not loose)

2. **I2C Data Connections**
   - [ ] DS3231 SDA connected to ESP32 **GPIO 2**
   - [ ] DS3231 SCL connected to ESP32 **GPIO 1**
   - [ ] No swapped connections (SDA↔SCL)

3. **Pull-up Resistors**
   - [ ] Check if your DS3231 module has built-in pull-up resistors (most do)
   - [ ] If not built-in, add 4.7kΩ - 10kΩ resistors:
     - One resistor between SDA and VCC
     - One resistor between SCL and VCC

4. **Module Status**
   - [ ] DS3231 LED (if present) is lit, indicating power
   - [ ] Module is not damaged
   - [ ] All solder joints are good (if DIY assembly)

5. **ESP32 Pins**
   - [ ] GPIO 1 and GPIO 2 are not being used by other peripherals
   - [ ] Pins are not damaged or shorted

---

## Testing Procedure

### Step 1: Verify Hardware with clock_test

**Before trying to set the clock**, verify your hardware works by running the `clock_test` application:

```bash
cd tools/test_apps
./build-test.sh clock_test flash

# Then connect with PuTTY or another terminal to view output
```

**Expected Output** (if hardware is working):
```
I (xxx) CLOCK_TEST: Found device at address: 0x68
I (xxx) CLOCK_TEST: Current time: 2024-01-15 14:30:45
```

**If clock_test shows "No I2C devices found"**:
- Hardware problem - review checklist above
- Try different GPIO pins if available
- Test with a known-good DS3231 module

**If clock_test successfully reads the time**:
- Hardware is working! Proceed to Step 2

### Step 2: Set Up Serial Terminal (PuTTY on Windows)

1. **Download PuTTY** from https://www.putty.org/
2. **Find your COM port**:
   - Open Device Manager
   - Expand "Ports (COM & LPT)"
   - Find "Silicon Labs CP210x" or "USB Serial Port" (note COM number, e.g., COM3)
3. **Configure PuTTY**:
   - Connection type: **Serial**
   - Serial line: **COM3** (or your COM port)
   - Speed: **115200**
4. **Click "Open"**

### Step 3: Flash and Run clock_set

```bash
cd tools/test_apps
./build-test.sh clock_set flash
```

After flashing, the application will start automatically and display prompts in PuTTY.

### Step 4: Set the Time

Follow the on-screen prompts in PuTTY to enter:
- Year (2000-2099)
- Month (1-12)
- Day (1-31)
- Hour (0-23, 24-hour format)
- Minute (0-59)
- Second (0-59)

Type `y` when prompted to confirm.

---

## Common Issues

### "No I2C devices found" after checking all hardware

**Try this**:
1. Swap SDA and SCL connections (they might be labeled incorrectly on your module)
2. Try different GPIO pins by editing `components/weather_common/include/hardware_config.h`:
   ```c
   #define HW_I2C_SDA_PIN 8   // Try different pins
   #define HW_I2C_SCL_PIN 9
   ```
3. Test I2C with a different device (e.g., I2C OLED display) to rule out ESP32 issues
4. Measure voltage at DS3231 VCC pin (should be 3.3V or 5V depending on your connection)

### "Cannot enter text" even with PuTTY

**Check**:
- Correct COM port selected
- Baud rate set to 115200
- Connection type is "Serial" not "SSH" or "Telnet"
- Cable is not charge-only (must be data cable)

### "Failed to write to DS3231" (after successfully detecting device)

**This means**:
- Hardware IS working (device found)
- Write operation failing - could be:
  - DS3231 control register issue
  - Need to clear oscillator stop flag
  - Module in read-only mode (rare)

**Try**:
- Disconnect and reconnect power to DS3231
- Remove and reinsert CR2032 battery (if present)
- Run clock_test to see if reading works

### Time not retained after power off

**Solution**:
- Install CR2032 battery in DS3231 battery holder
- Battery must be fresh (3V)
- Check battery holder contacts

---

## Alternative: Manual Time Setting via Code

If interactive input continues to have problems, you can temporarily hardcode the time:

1. Edit `tools/test_apps/clock_set/main/set_clock.c`
2. Comment out the input prompts and add:
   ```c
   // Hardcoded time (temporary for testing)
   datetime_t new_time = {
       .year = 2025,
       .month = 10,
       .day = 19,
       .hour = 15,
       .minute = 45,
       .second = 0
   };
   ```
3. Rebuild and flash

This bypasses the stdin issue entirely for testing purposes.

---

## Getting More Help

If you're still having issues:

1. **Run I2C bus scan** - the improved clock_set will show all detected devices
2. **Check clock_test output** - does it show any I2C devices?
3. **Verify GPIO pins** - are GPIO 1 and 2 the correct physical pins on your board?
4. **Test continuity** - use a multimeter to verify connections
5. **Try a different DS3231 module** - module might be faulty

## Reference

- **ESP32-S3 Pinout**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/
- **DS3231 Datasheet**: https://datasheets.maximintegrated.com/en/ds/DS3231.pdf
- **I2C Pull-up Calculator**: http://www.ti.com/lit/an/slva689/slva689.pdf
