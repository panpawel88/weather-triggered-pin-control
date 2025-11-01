#ifndef WEATHER_DIAGNOSTICS_H
#define WEATHER_DIAGNOSTICS_H

#include "esp_err.h"
#include "weather_fetch.h"

/**
 * @brief Send weather diagnostics data to remote HTTP server
 *
 * This function sends detailed weather forecast data to the diagnostics endpoint
 * configured in config.h (REMOTE_DIAGNOSTICS_URL). It includes:
 * - Hourly cloudcover values for tomorrow's daytime hours
 * - Sunrise and sunset times
 * - Calculated average cloudcover
 * - Resulting pin_off_hour and LED count
 *
 * WiFi must be initialized and connected before calling this function.
 * The feature must be enabled via HW_WEATHER_DIAGNOSTICS_ENABLED in hardware_config.h
 *
 * @param weather_data Pointer to weather_data_t containing forecast data
 * @param pin_off_hour The calculated pin-off hour based on cloudcover
 * @param led_count The calculated LED count based on cloudcover
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t send_weather_diagnostics(const weather_data_t *weather_data, int pin_off_hour, int led_count);

#endif // WEATHER_DIAGNOSTICS_H
