#include "weather_fetch.h"
#include "hardware_config.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "WEATHER_FETCH";

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    static int output_len;
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Copy data for both chunked and non-chunked responses
            if (evt->user_data) {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                output_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            output_len = 0;
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t fetch_weather_forecast(float latitude, float longitude, weather_data_t *weather_data) {
    if (!weather_data) {
        return ESP_ERR_INVALID_ARG;
    }

    weather_data->valid = false;
    weather_data->tomorrow_cloudcover = 0.0f;
    weather_data->num_daytime_hours = 0;
    weather_data->sunrise_hour = -1;
    weather_data->sunrise_minute = -1;
    weather_data->sunset_hour = -1;
    weather_data->sunset_minute = -1;
    weather_data->tomorrow_date[0] = '\0';
    memset(weather_data->daytime_hours, 0, sizeof(weather_data->daytime_hours));
    memset(weather_data->hourly_cloudcover, 0, sizeof(weather_data->hourly_cloudcover));

    char url[256];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f&daily=sunrise,sunset&hourly=cloudcover&forecast_days=2&timezone=auto",
             latitude, longitude);

    ESP_LOGI(TAG, "Fetching weather from: %s", url);

    char response_buffer[2048] = {0};

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = response_buffer,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                status_code, esp_http_client_get_content_length(client));

        if (status_code == 200) {
            cJSON *json = cJSON_Parse(response_buffer);
            if (json) {
                // Parse daily sunrise/sunset data first
                int sunrise_hour = -1;
                int sunrise_minute = -1;
                int sunset_hour = -1;
                int sunset_minute = -1;
                char tomorrow_date[11] = {0};  // "2025-10-20"

                cJSON *daily = cJSON_GetObjectItem(json, "daily");
                if (daily) {
                    cJSON *daily_time = cJSON_GetObjectItem(daily, "time");
                    cJSON *sunrise_array = cJSON_GetObjectItem(daily, "sunrise");
                    cJSON *sunset_array = cJSON_GetObjectItem(daily, "sunset");

                    if (daily_time && cJSON_IsArray(daily_time) &&
                        sunrise_array && cJSON_IsArray(sunrise_array) &&
                        sunset_array && cJSON_IsArray(sunset_array)) {

                        int daily_size = cJSON_GetArraySize(daily_time);
                        if (daily_size >= 2) {
                            // Get tomorrow's date (index 1)
                            cJSON *tomorrow_date_item = cJSON_GetArrayItem(daily_time, 1);
                            if (tomorrow_date_item && cJSON_IsString(tomorrow_date_item)) {
                                strncpy(tomorrow_date, cJSON_GetStringValue(tomorrow_date_item), 10);
                                strncpy(weather_data->tomorrow_date, tomorrow_date, 10);
                                weather_data->tomorrow_date[10] = '\0';
                            }

                            // Get tomorrow's sunrise (index 1)
                            cJSON *sunrise_item = cJSON_GetArrayItem(sunrise_array, 1);
                            if (sunrise_item && cJSON_IsString(sunrise_item)) {
                                const char *sunrise_str = cJSON_GetStringValue(sunrise_item);
                                // Extract hour and minute from ISO 8601: "2025-10-20T06:23"
                                sscanf(sunrise_str + 11, "%d:%d", &sunrise_hour, &sunrise_minute);
                                weather_data->sunrise_hour = sunrise_hour;
                                weather_data->sunrise_minute = sunrise_minute;
                            }

                            // Get tomorrow's sunset (index 1)
                            cJSON *sunset_item = cJSON_GetArrayItem(sunset_array, 1);
                            if (sunset_item && cJSON_IsString(sunset_item)) {
                                const char *sunset_str = cJSON_GetStringValue(sunset_item);
                                // Extract hour and minute from ISO 8601: "2025-10-20T18:47"
                                sscanf(sunset_str + 11, "%d:%d", &sunset_hour, &sunset_minute);
                                weather_data->sunset_hour = sunset_hour;
                                weather_data->sunset_minute = sunset_minute;
                            }
                        }
                    }
                }

                // Default to 6 AM - 6 PM if sunrise/sunset parsing failed
                int start_hour = 6;
                if (sunrise_hour >= 0 && sunrise_minute >= 0) {
                    // Round up: if minute >= 30, add 2 hours; otherwise add 1 hour
                    start_hour = (sunrise_minute >= 30) ? (sunrise_hour + 2) : (sunrise_hour + 1);
                }
                int end_hour = (sunset_hour >= 0) ? (sunset_hour - 1) : 18;

                ESP_LOGI(TAG, "Tomorrow's date: %s, sunrise: %02d:%02d, sunset: %02d:%02d",
                        tomorrow_date[0] ? tomorrow_date : "unknown",
                        sunrise_hour, sunrise_minute, sunset_hour, sunset_minute);
                ESP_LOGI(TAG, "Using hour range for averaging: %d - %d", start_hour, end_hour);

                // Parse hourly cloudcover data
                cJSON *hourly = cJSON_GetObjectItem(json, "hourly");
                if (hourly) {
                    cJSON *time_array = cJSON_GetObjectItem(hourly, "time");
                    cJSON *cloudcover_array = cJSON_GetObjectItem(hourly, "cloudcover");

                    if (time_array && cJSON_IsArray(time_array) &&
                        cloudcover_array && cJSON_IsArray(cloudcover_array)) {

                        int array_size = cJSON_GetArraySize(time_array);

                        // Find tomorrow's date by checking if we've moved past hour 0 after the first day
                        char first_date[11] = {0};  // "2025-10-19"
                        bool found_tomorrow = false;
                        float cloudcover_sum = 0.0f;
                        int daytime_count = 0;

                        for (int i = 0; i < array_size; i++) {
                            cJSON *time_item = cJSON_GetArrayItem(time_array, i);
                            cJSON *cloudcover_item = cJSON_GetArrayItem(cloudcover_array, i);

                            if (time_item && cJSON_IsString(time_item) &&
                                cloudcover_item && cJSON_IsNumber(cloudcover_item)) {

                                const char *timestamp = cJSON_GetStringValue(time_item);

                                // Extract date (first 10 chars: "2025-10-19")
                                if (i == 0) {
                                    strncpy(first_date, timestamp, 10);
                                }

                                // Check if this is tomorrow's data
                                if (strncmp(timestamp, first_date, 10) != 0) {
                                    found_tomorrow = true;

                                    // Extract hour from timestamp (e.g., "2025-10-20T14:00" -> 14)
                                    int hour = 0;
                                    sscanf(timestamp + 11, "%d", &hour);

                                    // Use dynamic daytime hours based on sunrise/sunset
                                    if (hour >= start_hour && hour <= end_hour) {
                                        float cloudcover_value = (float)cJSON_GetNumberValue(cloudcover_item);
                                        cloudcover_sum += cloudcover_value;

                                        // Store hourly data if within array bounds
                                        if (daytime_count < MAX_DAYTIME_HOURS) {
                                            weather_data->daytime_hours[daytime_count] = hour;
                                            weather_data->hourly_cloudcover[daytime_count] = cloudcover_value;
                                        }
                                        daytime_count++;
                                    }
                                }
                            }
                        }

                        // Store the final count (may be more than MAX_DAYTIME_HOURS but we only store up to the limit)
                        weather_data->num_daytime_hours = (daytime_count < MAX_DAYTIME_HOURS) ? daytime_count : MAX_DAYTIME_HOURS;

                        if (found_tomorrow && daytime_count > 0) {
                            weather_data->tomorrow_cloudcover = cloudcover_sum / daytime_count;
                            weather_data->valid = true;
                            ESP_LOGI(TAG, "Tomorrow daytime cloud cover: %.1f%% (avg of %d hours)",
                                    weather_data->tomorrow_cloudcover, daytime_count);
                        } else {
                            ESP_LOGE(TAG, "Failed to calculate tomorrow's daytime cloud cover");
                            err = ESP_FAIL;
                        }
                    } else {
                        ESP_LOGE(TAG, "Failed to parse hourly time or cloudcover arrays");
                        err = ESP_FAIL;
                    }
                } else {
                    ESP_LOGE(TAG, "Failed to parse hourly object");
                    err = ESP_FAIL;
                }
                cJSON_Delete(json);
            } else {
                ESP_LOGE(TAG, "Failed to parse JSON response");
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}
