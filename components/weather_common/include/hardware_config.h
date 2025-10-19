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
 * WiFi credentials are still in main/config.h (gitignored)
 */

// ============================================================================
// Hardware Pin Assignments
// ============================================================================

// Main control pin
#define HW_GPIO_CONTROL_PIN 13

// LED pins (active-low: 0=ON, 1=OFF)
#define HW_LED_PIN_1 5
#define HW_LED_PIN_2 6
#define HW_LED_PIN_3 7
#define HW_LED_PIN_4 15
#define HW_LED_PIN_5 16
#define HW_NUM_LEDS 5

// I2C pins for DS3231 RTC
#define HW_I2C_SDA_PIN 2
#define HW_I2C_SCL_PIN 1

// ============================================================================
// Weather Behavior Configuration
// ============================================================================

// Hour to check weather forecast (24h format)
#define HW_WEATHER_CHECK_HOUR 16  // 4 PM

// Default location (can be overridden in main/config.h)
#define HW_DEFAULT_LATITUDE  52.23
#define HW_DEFAULT_LONGITUDE 21.01

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
