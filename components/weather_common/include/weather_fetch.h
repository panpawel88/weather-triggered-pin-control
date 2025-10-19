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
 * @brief Initialize WiFi station mode
 *
 * This function initializes NVS, network interface, and WiFi.
 * It uses WIFI_SSID and WIFI_PASSWORD from config.h.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t weather_wifi_init(void);

/**
 * @brief Wait for WiFi connection
 *
 * @param max_retries Maximum number of retry attempts
 * @param retry_delay_ms Delay between retries in milliseconds
 * @return ESP_OK if connected, ESP_ERR_TIMEOUT if not connected after max retries
 */
esp_err_t weather_wifi_wait_connected(int max_retries, int retry_delay_ms);

/**
 * @brief Fetch weather forecast from Open-Meteo API
 *
 * This function connects to WiFi, fetches the weather forecast,
 * and returns tomorrow's cloud cover percentage.
 *
 * @param latitude Location latitude
 * @param longitude Location longitude
 * @param weather_data Pointer to weather_data_t to store the result
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t fetch_weather_forecast(float latitude, float longitude, weather_data_t *weather_data);

/**
 * @brief Shutdown WiFi to save power
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t weather_wifi_shutdown(void);

#endif // WEATHER_FETCH_H
