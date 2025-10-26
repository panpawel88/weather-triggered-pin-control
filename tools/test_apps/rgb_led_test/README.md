# RGB LED Deep Sleep Test Application

This test application verifies that the RGB LED state persists correctly across deep sleep cycles when using the RTC memory optimization (skipping re-initialization on wakeup).

## Purpose

This test validates the fix implemented in the main application where `rgb_led_init()` is only called once on first boot, and skipped on deep sleep wakeups to prevent the LED from flickering.

## Features

- **RTC Memory Tracking**: Uses `RTC_DATA_ATTR` to track initialization state across deep sleep
- **Short Sleep Cycles**: Wakes every 5 seconds for quick testing
- **3-Step Test Sequence**:
  1. Turn LED ON (red)
  2. Turn LED OFF
  3. Turn LED ON again (tests state preservation)
- **Automatic Cycling**: Repeats the test sequence continuously

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

### Cycle 1 (First Boot)
- **Step 1**: RGB LED initialization → LED turns ON (red)
- Deep sleep for 5 seconds

### Cycle 2
- **Step 2**: Skips initialization → LED turns OFF
- Deep sleep for 5 seconds

### Cycle 3
- **Step 3**: Skips initialization → LED turns ON (red)
- **CRITICAL**: LED should NOT flicker when turning on
- The LED was OFF before sleep and should remain OFF until `rgb_led_set_state(true)` is called
- Deep sleep for 5 seconds

### Cycle 4+
- Repeats the sequence (back to Step 1)

## What to Verify

1. **First boot only**: You should see "First boot - initializing RGB LED" message only once
2. **Subsequent wakeups**: You should see "Woke from deep sleep - skipping RGB LED initialization"
3. **No flicker in Cycle 3**: When the LED turns ON in Step 3, it should NOT briefly turn OFF first
4. **State preservation**: LED state should persist across sleep cycles

## Troubleshooting

### LED Doesn't Work

Check your `components/weather_common/include/hardware_config.h`:

```c
#define HW_RGB_LED_ENABLED true          // Must be true
#define HW_RGB_LED_GPIO 48               // Adjust to your board's GPIO
#define HW_RGB_LED_COLOR_R 255           // Red channel
#define HW_RGB_LED_COLOR_G 0             // Green channel
#define HW_RGB_LED_COLOR_B 0             // Blue channel
#define HW_RGB_LED_BRIGHTNESS 50         // Brightness (0-100)
```

**Common ESP32-S3 RGB LED pins:**
- GPIO 48: ESP32-S3-DevKitC-1 (most common)
- GPIO 38: Some ESP32-S3 variants
- GPIO 8: Other variants

### LED Flickers in Cycle 3

If you see the LED briefly turn OFF before turning ON in Step 3/Cycle 3:
- This indicates the initialization is being called on wakeup
- Check that `rgb_led_initialized` RTC variable is being set correctly
- Verify the logic in the main application matches this test

### Build Errors

- Ensure ESP-IDF is properly installed and sourced
- Check that the `weather_common` component is accessible
- Try `idf.py clean` then rebuild
- Verify you're targeting the correct chip (ESP32 or ESP32-S3)

## Configuration

You can adjust the sleep duration in `main/rgb_led_test.c`:

```c
#define SLEEP_DURATION_SEC 5  // Seconds between wake cycles
```

- Increase for slower testing (less frequent wakeups)
- Decrease for faster testing (more frequent wakeups)

## Technical Details

- **LED Driver**: Uses `rgb_led_control.c` from `weather_common` component
- **Protocol**: WS2812/NeoPixel (GRB color format via RMT)
- **RTC Memory**: Preserves `rgb_led_initialized` and `test_cycle` across deep sleep
- **Hardware State**: LED strip hardware maintains its state during deep sleep
- **Power Consumption**: Deep sleep significantly reduces power between wakeups

## Exit

Press **Ctrl+C** in the serial monitor to stop the test, or press the **RESET** button on your ESP32 board to start a fresh test cycle.
