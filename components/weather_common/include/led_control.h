#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdbool.h>
#include "driver/gpio.h"

/**
 * @brief Calculate number of LEDs to turn on based on cloud cover percentage
 *
 * Cloud cover ranges:
 * - 0-9%: 5 LEDs (Very clear)
 * - 10-19%: 4 LEDs (Clear)
 * - 20-29%: 3 LEDs (Mostly clear)
 * - 30-39%: 2 LEDs (Partly cloudy)
 * - 40-49%: 1 LED (Cloudy)
 * - 50-100%: 0 LEDs (Very cloudy)
 *
 * @param cloudcover Cloud cover percentage (0-100)
 * @return Number of LEDs to activate (0-5)
 */
int led_count_from_cloudcover(float cloudcover);

/**
 * @brief Control LEDs based on cloud cover and main pin state
 *
 * LEDs are only active when mainPinActive is true.
 * The number of LEDs lit indicates cloud cover levels.
 * LEDs use active-low logic (0=ON, 1=OFF).
 *
 * @param led_pins Array of GPIO pin numbers for LEDs
 * @param num_leds Number of LEDs in the array
 * @param mainPinActive Whether the main control pin is active
 * @param cloudcover Cloud cover percentage (0-100)
 */
void control_leds(const gpio_num_t *led_pins, int num_leds, bool mainPinActive, float cloudcover);

/**
 * @brief Set individual LED state (for testing)
 *
 * @param led_pin GPIO pin number for the LED
 * @param on true to turn LED on, false to turn it off
 */
void set_led(gpio_num_t led_pin, bool on);

/**
 * @brief Initialize all LEDs and turn them off
 *
 * @param led_pins Array of GPIO pin numbers for LEDs
 * @param num_leds Number of LEDs in the array
 */
void init_leds(const gpio_num_t *led_pins, int num_leds);

/**
 * @brief Set RTC GPIO output level with proper hold management
 *
 * This helper function handles the complete sequence needed to change
 * an RTC GPIO output state while preserving it during deep sleep:
 * - Disables GPIO hold
 * - Initializes GPIO
 * - Sets direction to output
 * - Sets the level
 * - Re-enables GPIO hold
 *
 * @param pin GPIO pin number
 * @param level Output level (0 or 1)
 */
void set_rtc_gpio_output(gpio_num_t pin, int level);

#endif // LED_CONTROL_H
