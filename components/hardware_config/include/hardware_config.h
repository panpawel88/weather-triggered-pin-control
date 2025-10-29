#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

/**
 * @file hardware_config.h
 * @brief Single source of truth for all hardware and behavior configuration
 *
 * This file defines ALL configuration for the weather control system:
 * - GPIO pin assignments
 * - Cloud cover ranges and pin-off hours
 * - Weather check schedule
 * - Default location
 *
 * IMPORTANT: This is the ONLY place to configure hardware and behavior!
 * Both the main application and all test applications use these same values.
 *
 * WiFi credentials are in config.h (same directory, gitignored)
 */

// ============================================================================
// Hardware Pin Assignments
// ============================================================================

// Main control pin
#define HW_GPIO_CONTROL_PIN 13

// LED pins (active-low: 0=ON, 1=OFF)
// To change number of LEDs: modify both HW_NUM_LEDS and HW_LED_PINS array
#define HW_NUM_LEDS 5
#define HW_LED_PINS {5, 6, 7, 15, 16}

// I2C pins for DS3231 RTC
#define HW_I2C_SDA_PIN 1
#define HW_I2C_SCL_PIN 2

// ============================================================================
// Weather Behavior Configuration
// ============================================================================

// Hour to check weather forecast (24h format)
#define HW_WEATHER_CHECK_HOUR 16  // 4 PM

// Default location (can be overridden in main/config.h)
#define HW_DEFAULT_LATITUDE  52.23
#define HW_DEFAULT_LONGITUDE 21.01

// ============================================================================
// Timezone Configuration
// ============================================================================
// POSIX TZ string for Central European Time with DST
// Format: CET-1CEST,M3.5.0,M10.5.0/3
//   CET-1: Standard time is UTC+1 (negative means east of UTC)
//   CEST: Daylight saving time name
//   M3.5.0: DST starts last (5th) Sunday (0) of March (3) at 2:00 AM
//   M10.5.0/3: DST ends last Sunday of October at 3:00 AM
#define HW_TIMEZONE_POSIX "CET-1CEST,M3.5.0,M10.5.0/3"

// RTC storage format: UTC (all times stored in RTC are UTC)
// User-facing times: Local time (CET/CEST) with automatic DST adjustment

// ============================================================================
// Remote Logging Configuration
// ============================================================================

// Enable/disable remote logging to HTTP server
#define HW_REMOTE_LOGGING_ENABLED true

// Maximum number of log messages to buffer (not bytes)
// Each message uses ~150 bytes, so 100 messages = ~15KB RAM
#define HW_LOG_BUFFER_SIZE 100

// Device identifier for remote logging (helps distinguish multiple devices)
#define HW_LOG_DEVICE_NAME "weather-esp32"

// Tag whitelist for remote logging (only these tags sent to server)
// To send all tags, set HW_REMOTE_LOG_TAG_COUNT to 0
// Serial logging always shows all tags regardless of this filter
#define HW_REMOTE_LOG_TAGS {"WEATHER_CONTROL", "RGB_LED"}
#define HW_REMOTE_LOG_TAG_COUNT 2

// ============================================================================
// Built-in RGB LED Configuration
// ============================================================================

// Enable/disable built-in RGB LED as GPIO control pin indicator
// When enabled, the RGB LED will mirror the GPIO control pin state:
// - LED ON (green) when GPIO_CONTROL_PIN is activated (9 AM to pin-off hour)
// - LED OFF when GPIO_CONTROL_PIN is deactivated
#define HW_RGB_LED_ENABLED true

// GPIO pin for built-in RGB LED (WS2812/NeoPixel)
// Common ESP32-S3 pins: 48 (most DevKits), 38, 8
#define HW_RGB_LED_GPIO 48

// RGB LED color when active (0-255 for each channel)
// Default: Green (success/on indicator)
#define HW_RGB_LED_COLOR_R 0
#define HW_RGB_LED_COLOR_G 255
#define HW_RGB_LED_COLOR_B 0

// RGB LED brightness (0-100)
// Default: 30% (medium brightness, clearly visible but not too bright)
#define HW_RGB_LED_BRIGHTNESS 30

// ============================================================================
// Cloud Cover Ranges Configuration
// ============================================================================
// Each range defines: [min%, max%) -> hour when pin turns off
//
// Example: Range 1 (0-10% cloud cover) -> pin stays on until 22:00 (10 PM)
//          Range 6 (50-100% cloud cover) -> pin stays on until 17:00 (5 PM)

typedef struct {
    float min_cloudcover;    // Minimum cloudcover percentage (inclusive)
    float max_cloudcover;    // Maximum cloudcover percentage (exclusive)
    int pin_high_until_hour; // Hour when pin goes low (24h format)
} cloudcover_range_t;

#define HW_NUM_CLOUDCOVER_RANGES 6

// Cloud cover ranges array
// IMPORTANT: Ranges must be consecutive and cover 0-100%
static const cloudcover_range_t HW_CLOUDCOVER_RANGES[HW_NUM_CLOUDCOVER_RANGES] = {
    // Very clear sky (0-9%)
    {0.0f,  10.0f, 22},   // Pin off at 10 PM

    // Clear sky (10-19%)
    {10.0f, 20.0f, 21},   // Pin off at 9 PM

    // Mostly clear (20-29%)
    {20.0f, 30.0f, 20},   // Pin off at 8 PM

    // Partly cloudy (30-39%)
    {30.0f, 40.0f, 19},   // Pin off at 7 PM

    // Cloudy (40-49%)
    {40.0f, 50.0f, 18},   // Pin off at 6 PM

    // Very cloudy (50-100%)
    {50.0f, 100.0f, 17}   // Pin off at 5 PM
};

#endif // HARDWARE_CONFIG_H
