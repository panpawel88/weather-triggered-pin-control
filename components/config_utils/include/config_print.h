#ifndef CONFIG_PRINT_H
#define CONFIG_PRINT_H

/**
 * @brief Print all hardware pin configuration
 *
 * Prints GPIO pin assignments for control pin, LEDs, and I2C.
 */
void print_hardware_config(void);

/**
 * @brief Print WiFi configuration (SSID only, password masked)
 *
 * Prints WiFi SSID and indicates password length.
 */
void print_wifi_config(void);

/**
 * @brief Print location configuration
 *
 * Prints latitude and longitude settings.
 */
void print_location_config(void);

/**
 * @brief Print weather check schedule
 *
 * Prints the hour when weather is checked.
 */
void print_weather_schedule(void);

/**
 * @brief Print cloud cover ranges configuration
 *
 * Prints all cloud cover ranges and their corresponding pin-off hours.
 */
void print_cloudcover_ranges(void);

/**
 * @brief Print all configuration at once
 *
 * Convenience function that calls all other print functions.
 */
void print_all_config(void);

#endif // CONFIG_PRINT_H
