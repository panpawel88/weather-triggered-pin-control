#ifndef LED_GPIO_H
#define LED_GPIO_H

#include <stdbool.h>
#include "driver/gpio.h"

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

#endif // LED_GPIO_H
