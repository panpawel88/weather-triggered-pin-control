#ifndef RGB_LED_CONTROL_H
#define RGB_LED_CONTROL_H

#include "hardware_config.h"
#include "esp_err.h"
#include <stdbool.h>

// Only compile RGB LED functions if the feature is enabled
#if HW_RGB_LED_ENABLED

/**
 * @brief Initialize the built-in RGB LED
 *
 * Configures the RMT peripheral and led_strip driver to control
 * the WS2812/NeoPixel RGB LED on the configured GPIO pin.
 *
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t rgb_led_init(void);

/**
 * @brief Set RGB LED state (on/off)
 *
 * When on=true, sets the LED to the configured color and brightness.
 * When on=false, turns the LED off (clears it).
 *
 * @param on true to turn LED on, false to turn it off
 * @return ESP_OK on success, ESP_FAIL if not initialized or error occurred
 */
esp_err_t rgb_led_set_state(bool on);

/**
 * @brief Clean up RGB LED resources
 *
 * Turns off the LED and releases resources.
 * Should be called before deep sleep (though not strictly necessary
 * as the LED will lose power anyway).
 *
 * @return ESP_OK on success
 */
esp_err_t rgb_led_deinit(void);

#else

// Stub functions when RGB LED is disabled (compile to nothing)
static inline esp_err_t rgb_led_init(void) { return ESP_OK; }
static inline esp_err_t rgb_led_set_state(bool on) { (void)on; return ESP_OK; }
static inline esp_err_t rgb_led_deinit(void) { return ESP_OK; }

#endif // HW_RGB_LED_ENABLED

#endif // RGB_LED_CONTROL_H
