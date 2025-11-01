#include "weather_diagnostics.h"
#include "hardware_config.h"
#include "rtc_helper.h"
#include "timezone_helper.h"
#include "cloudcover_leds.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include <string.h>
#include <stdio.h>

// Check if config.h exists and include it
#ifndef __has_include
    #error "Compiler does not support __has_include"
#endif

#if !__has_include("config.h")
    #error "config.h not found! Please copy components/hardware_config/include/config.h.example to components/hardware_config/include/config.h"
#endif
#include "config.h"

// Provide default URL if not defined (to allow gradual migration)
#ifndef REMOTE_DIAGNOSTICS_URL
    #define REMOTE_DIAGNOSTICS_URL "http://192.168.1.100:3000/api/diagnostics"
    #warning "REMOTE_DIAGNOSTICS_URL not defined in config.h, using default. Update your config.h from config.h.example"
#endif

static const char *TAG = "WEATHER_DIAG";

// Get current timestamp from RTC (in local time)
static void get_timestamp(char *timestamp_str) {
    datetime_t utc_time, local_time;

    // Read UTC time from RTC
    if (rtc_read_time(&utc_time) != ESP_OK) {
        strcpy(timestamp_str, "0000-00-00 00:00:00");
        return;
    }

    // Convert to local time
    if (utc_to_local(&utc_time, &local_time) != ESP_OK) {
        strcpy(timestamp_str, "0000-00-00 00:00:00");
        return;
    }

    snprintf(timestamp_str, 20, "%04d-%02d-%02d %02d:%02d:%02d",
             local_time.year, local_time.month, local_time.day,
             local_time.hour, local_time.minute, local_time.second);
}

esp_err_t send_weather_diagnostics(const weather_data_t *weather_data, int pin_off_hour, int led_count) {
#if !HW_WEATHER_DIAGNOSTICS_ENABLED
    // Feature disabled, return success without doing anything
    return ESP_OK;
#else
    if (!weather_data || !weather_data->valid) {
        ESP_LOGW(TAG, "Invalid weather data, skipping diagnostics");
        return ESP_ERR_INVALID_ARG;
    }

    // Get current timestamp
    char timestamp[20];
    get_timestamp(timestamp);

    // Build JSON payload
    // Estimate: Base (~150) + hourly data (num_hours * ~30) + safety margin
    int json_size = 512 + (weather_data->num_daytime_hours * 40);
    char *json_payload = malloc(json_size);
    if (!json_payload) {
        ESP_LOGE(TAG, "Failed to allocate JSON buffer (%d bytes)", json_size);
        return ESP_FAIL;
    }

    int offset = 0;

    // Build JSON header
    offset += snprintf(json_payload + offset, json_size - offset,
                      "{\"device\":\"%s\",\"timestamp\":\"%s\",\"date\":\"%s\","
                      "\"sunrise\":\"%02d:%02d\",\"sunset\":\"%02d:%02d\","
                      "\"avg_cloudcover\":%.1f,\"pin_off_hour\":%d,\"led_count\":%d,\"hourly\":[",
                      HW_LOG_DEVICE_NAME, timestamp, weather_data->tomorrow_date,
                      weather_data->sunrise_hour, weather_data->sunrise_minute,
                      weather_data->sunset_hour, weather_data->sunset_minute,
                      weather_data->tomorrow_cloudcover, pin_off_hour, led_count);

    // Add hourly cloudcover data
    for (int i = 0; i < weather_data->num_daytime_hours && offset < json_size - 50; i++) {
        offset += snprintf(json_payload + offset, json_size - offset,
                          "%s{\"hour\":%d,\"cloudcover\":%.1f}",
                          i > 0 ? "," : "",
                          weather_data->daytime_hours[i],
                          weather_data->hourly_cloudcover[i]);
    }

    offset += snprintf(json_payload + offset, json_size - offset, "]}");

    ESP_LOGI(TAG, "Sending diagnostics (%d bytes): %s", offset, json_payload);

    // Send HTTP POST
    esp_http_client_config_t config = {
        .url = REMOTE_DIAGNOSTICS_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        free(json_payload);
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            ESP_LOGI(TAG, "Diagnostics sent successfully (HTTP %d)", status_code);
        } else {
            ESP_LOGW(TAG, "Server returned HTTP %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(json_payload);
    return err;
#endif // HW_WEATHER_DIAGNOSTICS_ENABLED
}
