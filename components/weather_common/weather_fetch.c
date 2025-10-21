#include "weather_fetch.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include <string.h>

// Include config.h for WiFi credentials (if available)
#if __has_include("config.h")
    #include "config.h"
#endif

// Provide default WiFi credentials if not defined in config.h
#ifndef WIFI_SSID
    #define WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASSWORD
    #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

static const char *TAG = "WEATHER_FETCH";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "WiFi connected");
    }
}

esp_err_t weather_wifi_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialized");
    return ESP_OK;
}

esp_err_t weather_wifi_wait_connected(int max_retries, int retry_delay_ms) {
    esp_netif_ip_info_t ip_info;
    int retry_count = 0;

    while (retry_count < max_retries) {
        if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info) == ESP_OK) {
            if (ip_info.ip.addr != 0) {
                ESP_LOGI(TAG, "WiFi connected, IP: " IPSTR, IP2STR(&ip_info.ip));
                return ESP_OK;
            }
        }
        vTaskDelay(retry_delay_ms / portTICK_PERIOD_MS);
        retry_count++;
    }

    ESP_LOGE(TAG, "WiFi connection timeout");
    return ESP_ERR_TIMEOUT;
}

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
                int sunset_hour = -1;
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
                            }

                            // Get tomorrow's sunrise (index 1)
                            cJSON *sunrise_item = cJSON_GetArrayItem(sunrise_array, 1);
                            if (sunrise_item && cJSON_IsString(sunrise_item)) {
                                const char *sunrise_str = cJSON_GetStringValue(sunrise_item);
                                // Extract hour from ISO 8601: "2025-10-20T06:23"
                                sscanf(sunrise_str + 11, "%d", &sunrise_hour);
                            }

                            // Get tomorrow's sunset (index 1)
                            cJSON *sunset_item = cJSON_GetArrayItem(sunset_array, 1);
                            if (sunset_item && cJSON_IsString(sunset_item)) {
                                const char *sunset_str = cJSON_GetStringValue(sunset_item);
                                // Extract hour from ISO 8601: "2025-10-20T18:47"
                                sscanf(sunset_str + 11, "%d", &sunset_hour);
                            }
                        }
                    }
                }

                // Default to 6 AM - 6 PM if sunrise/sunset parsing failed
                int start_hour = (sunrise_hour >= 0) ? (sunrise_hour + 1) : 6;
                int end_hour = (sunset_hour >= 0) ? (sunset_hour - 1) : 18;

                ESP_LOGI(TAG, "Tomorrow's date: %s, sunrise hour: %d, sunset hour: %d",
                        tomorrow_date[0] ? tomorrow_date : "unknown", sunrise_hour, sunset_hour);
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
                                        cloudcover_sum += (float)cJSON_GetNumberValue(cloudcover_item);
                                        daytime_count++;
                                    }
                                }
                            }
                        }

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

esp_err_t weather_wifi_shutdown(void) {
    esp_err_t err;

    err = esp_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi stop failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi deinit failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "WiFi shutdown complete");
    return ESP_OK;
}
