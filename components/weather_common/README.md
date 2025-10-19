# Weather Common Component

Shared component library for weather triggered pin control system.

## Single Source of Truth Configuration

This component uses **`include/hardware_config.h`** as the single source of truth for ALL configuration:

✅ GPIO pin assignments
✅ Cloud cover ranges and pin-off hours
✅ Weather check schedule
✅ Default location (can be overridden in main/config.h)

### Why This Approach?

- **Simple** - One file to edit, no menuconfig needed
- **Consistent** - Production app and ALL test apps use identical configuration
- **Clear** - Everything in one readable header file with comments
- **Version controlled** - Configuration tracked in git

### How It Works

1. **hardware_config.h** - ALL hardware and behavior configuration
   - GPIO pins for control pin, LEDs, I2C
   - Cloud cover ranges (6 ranges from 0-100%)
   - Weather check hour (when to fetch forecast)
   - Default latitude/longitude

2. **Main application** - Uses hardware_config.h directly
   - No menuconfig needed
   - Simple and straightforward

3. **Test applications** - Use hardware_config.h directly
   - **Always test the same configuration as production**
   - Zero configuration needed

## Updating Configuration

To change your hardware or behavior:

1. Edit `components/weather_common/include/hardware_config.h`
2. Update any `HW_*` values or cloud cover ranges
3. Rebuild all applications

**That's it!** Both main app and all 5 test apps automatically use the new configuration.

## Component Structure

```
weather_common/
├── include/
│   ├── hardware_config.h    # Hardware pin assignments (single source of truth)
│   ├── rtc_helper.h          # DS3231 RTC functions
│   ├── led_control.h         # LED control functions
│   ├── weather_fetch.h       # Weather API functions
│   └── config_print.h        # Configuration display utilities
├── rtc_helper.c
├── led_control.c
├── weather_fetch.c
├── config_print.c
├── CMakeLists.txt
├── Kconfig                    # menuconfig options
└── README.md                  # This file
```

## Dependencies

- ESP-IDF driver components (I2C, GPIO, RTC)
- ESP WiFi, HTTP client, JSON
- WiFi credentials from `main/config.h` (created from config.h.example)
