# RGB LED Test Application

This test application verifies the built-in WS2812/NeoPixel RGB LED on ESP32-S3 development boards by displaying a smooth rainbow color effect.

## Features

- **Rainbow Effect**: Smooth color cycling through the entire spectrum
- **Auto-configurable GPIO**: Defaults to the most common pin (GPIO 48)
- **Clear Diagnostics**: Helpful messages to troubleshoot if the LED doesn't work
- **HSV Color Space**: Smooth, visually pleasing color transitions

## Quick Start

### Using the Build Script (Recommended)

**macOS/Linux:**
```bash
cd tools/test_apps
./build-test.sh rgb_led_test flash-monitor
```

**Windows:**
```cmd
cd tools\test_apps
build-test.bat rgb_led_test flash-monitor
```

### Manual Build

```bash
cd tools/test_apps/rgb_led_test
idf.py build flash monitor
```

## Expected Behavior

When running successfully, you should see:
1. The RGB LED smoothly cycling through rainbow colors (red → yellow → green → cyan → blue → magenta → red)
2. Serial output showing hue values and RGB components every 30 degrees

## Troubleshooting

### LED Doesn't Light Up

If the LED doesn't change colors:

1. **Try different GPIO pins**: Edit `main/rgb_led_test.c` and change the `RGB_LED_GPIO` define:
   ```c
   #define RGB_LED_GPIO 48  // Try 38 or 8 if this doesn't work
   ```

2. **Common ESP32-S3 RGB LED pins**:
   - **GPIO 48**: ESP32-S3-DevKitC-1 (most common)
   - **GPIO 38**: Some ESP32-S3 boards
   - **GPIO 8**: Other variants

3. **Check your board documentation**: Look up your specific ESP32-S3 board model to find the RGB LED pin

4. **Verify your board has an RGB LED**: Look for a small LED on the board, often labeled "RGB" or with "WS2812" printed nearby

### Build Errors

If you get compilation errors:

- Ensure ESP-IDF is properly installed and sourced
- Try `idf.py clean` then rebuild
- Check that you're targeting ESP32-S3: `idf.py set-target esp32s3`

## Configuration

You can adjust the rainbow effect in `main/rgb_led_test.c`:

```c
#define RAINBOW_HUE_STEP 1      // Color change speed (1-360)
#define RAINBOW_DELAY_MS 20     // Delay between updates (ms)
#define BRIGHTNESS 50           // LED brightness (0-100)
```

- **Faster rainbow**: Increase `RAINBOW_HUE_STEP` or decrease `RAINBOW_DELAY_MS`
- **Slower rainbow**: Decrease `RAINBOW_HUE_STEP` or increase `RAINBOW_DELAY_MS`
- **Brighter/dimmer**: Adjust `BRIGHTNESS` (0-100)

## Technical Details

- **LED Driver**: ESP-IDF's `led_strip` component with RMT peripheral
- **Protocol**: WS2812/NeoPixel (GRB color format)
- **Color Space**: HSV (Hue, Saturation, Value) converted to RGB
- **RMT Frequency**: 10 MHz
- **ESP32-S3 Specific**: Uses `mem_block_symbols = 0` for auto-configuration (ESP32-S3 requires 48 symbols, not the default 64)

### Important Note for ESP32-S3

This test app includes a critical fix for ESP32-S3 boards: the `mem_block_symbols` field must be set to 0 (for auto-config) or 48 (ESP32-S3 specific). Without this setting, the LED strip driver will not work on ESP32-S3 boards due to the default value of 64 being incompatible.

## Exit

Press **Ctrl+C** in the serial monitor to stop the test.
