#include <stdio.h>
#include "esp_log.h"
#include "weather_fetch.h"
#include "wifi_helper.h"
#include "led_control.h"
#include "timezone_helper.h"
#include "hardware_config.h"

// Include config.h for location overrides
#if __has_include("config.h")
    #include "config.h"
#endif

static const char *TAG = "WEATHER_TEST";

// Location configuration: Use config.h overrides if defined, otherwise use hardware defaults
#ifndef LATITUDE
    #define LATITUDE HW_DEFAULT_LATITUDE
#endif
#ifndef LONGITUDE
    #define LONGITUDE HW_DEFAULT_LONGITUDE
#endif

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Weather Forecast Test Application");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Initialize timezone for DST support
    ESP_LOGI(TAG, "Initializing timezone: %s", HW_TIMEZONE_POSIX);
    if (timezone_init() != ESP_OK) {
        ESP_LOGE(TAG, "Timezone initialization failed!");
        return;
    }
    ESP_LOGI(TAG, "Timezone initialized successfully");
    ESP_LOGI(TAG, "");

    ESP_LOGI(TAG, "Location: Latitude=%.6f, Longitude=%.6f", LATITUDE, LONGITUDE);
    ESP_LOGI(TAG, "");

    // Initialize WiFi and connect
    ESP_LOGI(TAG, "Initializing WiFi...");
    if (wifi_init() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed!");
        return;
    }

    // Wait for connection
    ESP_LOGI(TAG, "Waiting for WiFi connection and IP address...");
    if (wifi_wait_connected(60, 500) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi/DHCP timeout!");
        wifi_shutdown();
        return;
    }

    ESP_LOGI(TAG, "WiFi connected successfully");
    ESP_LOGI(TAG, "");

    // Fetch weather forecast
    ESP_LOGI(TAG, "Fetching weather forecast from Open-Meteo API...");
    weather_data_t weather_data;
    if (fetch_weather_forecast(LATITUDE, LONGITUDE, &weather_data) == ESP_OK) {
        if (weather_data.valid) {
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "  Weather Forecast Results");
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "Tomorrow's cloud cover: %.1f%%", weather_data.tomorrow_cloudcover);
            ESP_LOGI(TAG, "LED count: %d LEDs", led_count_from_cloudcover(weather_data.tomorrow_cloudcover));
            ESP_LOGI(TAG, "");

            // Interpret cloud cover
            const char *condition;
            if (weather_data.tomorrow_cloudcover >= 50.0f) condition = "Very cloudy";
            else if (weather_data.tomorrow_cloudcover >= 40.0f) condition = "Cloudy";
            else if (weather_data.tomorrow_cloudcover >= 30.0f) condition = "Partly cloudy";
            else if (weather_data.tomorrow_cloudcover >= 20.0f) condition = "Mostly clear";
            else if (weather_data.tomorrow_cloudcover >= 10.0f) condition = "Clear";
            else condition = "Very clear";

            ESP_LOGI(TAG, "Weather condition: %s", condition);
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "========================================");
        } else {
            ESP_LOGE(TAG, "Weather data is invalid");
        }
    } else {
        ESP_LOGE(TAG, "Failed to fetch weather forecast");
    }

    // Shutdown WiFi
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Shutting down WiFi...");
    wifi_shutdown();
    ESP_LOGI(TAG, "Test complete");
}
