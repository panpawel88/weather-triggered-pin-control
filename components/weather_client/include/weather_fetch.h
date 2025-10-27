#ifndef WEATHER_FETCH_H
#define WEATHER_FETCH_H

#include "esp_err.h"
#include <stdbool.h>

// Weather data structure
typedef struct {
    float tomorrow_cloudcover;  // Cloud cover percentage for tomorrow (0-100)
    bool valid;                  // Whether the data is valid
} weather_data_t;

/**
 * @brief Fetch weather forecast from Open-Meteo API
 *
 * This function fetches the weather forecast and returns tomorrow's cloud cover percentage.
 * WiFi must be initialized and connected before calling this function.
 *
 * @param latitude Location latitude
 * @param longitude Location longitude
 * @param weather_data Pointer to weather_data_t to store the result
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t fetch_weather_forecast(float latitude, float longitude, weather_data_t *weather_data);

#endif // WEATHER_FETCH_H
