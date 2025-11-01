#ifndef WEATHER_FETCH_H
#define WEATHER_FETCH_H

#include "esp_err.h"
#include <stdbool.h>

// Maximum number of daytime hours (conservative estimate: 18 hours max)
#define MAX_DAYTIME_HOURS 18

// Weather data structure
typedef struct {
    float tomorrow_cloudcover;  // Cloud cover percentage for tomorrow (0-100)
    bool valid;                  // Whether the data is valid

    // Diagnostic data (only populated if HW_WEATHER_DIAGNOSTICS_ENABLED is true)
    int daytime_hours[MAX_DAYTIME_HOURS];      // Hour values for daytime (e.g., [7, 8, 9, ..., 17])
    float hourly_cloudcover[MAX_DAYTIME_HOURS]; // Cloudcover for each daytime hour
    int num_daytime_hours;                      // Number of valid daytime hours
    int sunrise_hour;                           // Tomorrow's sunrise hour (24h format)
    int sunrise_minute;                         // Tomorrow's sunrise minute
    int sunset_hour;                            // Tomorrow's sunset hour (24h format)
    int sunset_minute;                          // Tomorrow's sunset minute
    char tomorrow_date[11];                     // Tomorrow's date "YYYY-MM-DD"
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
