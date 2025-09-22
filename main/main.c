#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "driver/i2c.h"
#include "nvs_flash.h"
#include "cJSON.h"

static const char *TAG = "WEATHER_CONTROL";

// Configuration
#define WIFI_SSID "YOUR_NETWORK"
#define WIFI_PASSWORD "YOUR_PASSWORD"
#define LATITUDE 52.23f  // Warsaw, Poland
#define LONGITUDE 21.01f

// Hardware
#define GPIO_CONTROL_PIN GPIO_NUM_2
#define WEATHER_CHECK_HOUR 16  // 4 PM

// I2C configuration for DS3231 RTC
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define DS3231_ADDR 0x68

// RTC memory variables persist during deep sleep
RTC_DATA_ATTR bool tomorrowSunny = false;
RTC_DATA_ATTR bool weatherFetched = false;

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} datetime_t;

// Convert BCD to decimal
static uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}


esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

esp_err_t ds3231_read_time(datetime_t *dt) {
    uint8_t data[7];
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, DS3231_ADDR,
                                               (uint8_t[]){0x00}, 1, data, 7, 1000 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        return ret;
    }

    dt->second = bcd_to_dec(data[0] & 0x7F);
    dt->minute = bcd_to_dec(data[1] & 0x7F);
    dt->hour = bcd_to_dec(data[2] & 0x3F);
    dt->day = bcd_to_dec(data[4] & 0x3F);
    dt->month = bcd_to_dec(data[5] & 0x1F);
    dt->year = bcd_to_dec(data[6]) + 2000;

    return ESP_OK;
}

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

esp_err_t wifi_init_sta(void) {
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

    return ESP_OK;
}

esp_err_t http_event_handler(esp_http_client_event_t *evt) {
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
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                    output_len += evt->data_len;
                }
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

void fetchWeatherForecast(void) {
    ESP_LOGI(TAG, "Starting weather fetch");

    if (wifi_init_sta() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed");
        return;
    }

    // Wait for WiFi connection (check for IP)
    int retry_count = 0;
    esp_netif_ip_info_t ip_info;
    while (retry_count < 20) {
        if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info) == ESP_OK) {
            if (ip_info.ip.addr != 0) {
                break;
            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
        retry_count++;
    }

    char url[256];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f&daily=cloudcover&forecast_days=2",
             LATITUDE, LONGITUDE);

    char response_buffer[1024] = {0};

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = response_buffer,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));

        cJSON *json = cJSON_Parse(response_buffer);
        if (json) {
            cJSON *daily = cJSON_GetObjectItem(json, "daily");
            if (daily) {
                cJSON *cloudcover = cJSON_GetObjectItem(daily, "cloudcover");
                if (cloudcover && cJSON_IsArray(cloudcover)) {
                    cJSON *tomorrow = cJSON_GetArrayItem(cloudcover, 1);
                    if (tomorrow && cJSON_IsNumber(tomorrow)) {
                        float cloud_cover = (float)cJSON_GetNumberValue(tomorrow);
                        tomorrowSunny = cloud_cover < 50.0f;
                        ESP_LOGI(TAG, "Tomorrow cloud cover: %.1f%%, sunny: %s",
                                cloud_cover, tomorrowSunny ? "yes" : "no");
                    }
                }
            }
            cJSON_Delete(json);
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    esp_wifi_stop();
    esp_wifi_deinit();
}

void controlGPIO(datetime_t *now) {
    int hour = now->hour;
    bool activate = false;

    if (tomorrowSunny) {
        activate = (hour >= 9 && hour < 20);  // 9 AM - 8 PM
    } else {
        activate = (hour >= 9 && hour < 17);  // 9 AM - 5 PM
    }

    ESP_LOGI(TAG, "GPIO control: hour=%d, sunny=%s, activate=%s",
             hour, tomorrowSunny ? "yes" : "no", activate ? "yes" : "no");

    rtc_gpio_init(GPIO_CONTROL_PIN);
    rtc_gpio_set_direction(GPIO_CONTROL_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(GPIO_CONTROL_PIN, activate ? 1 : 0);
    rtc_gpio_hold_en(GPIO_CONTROL_PIN);  // Maintain state during sleep
}

void app_main(void) {
    ESP_LOGI(TAG, "Weather Triggered Pin Control starting");

    // Initialize I2C for RTC
    if (i2c_master_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed, restarting");
        esp_restart();
    }

    datetime_t now;
    if (ds3231_read_time(&now) != ESP_OK) {
        ESP_LOGE(TAG, "RTC read failed, restarting");
        esp_restart();
    }

    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
             now.year, now.month, now.day, now.hour, now.minute, now.second);

    // Fetch weather at 4 PM
    if (now.hour == WEATHER_CHECK_HOUR && !weatherFetched) {
        fetchWeatherForecast();
        weatherFetched = true;
    }

    // Control GPIO based on weather and time
    controlGPIO(&now);

    // Reset fetch flag at midnight
    if (now.hour == 0) {
        weatherFetched = false;
    }

    ESP_LOGI(TAG, "Going to deep sleep for 1 hour");

    // Deep sleep for 1 hour
    esp_sleep_enable_timer_wakeup(3600 * 1000000ULL);
    esp_deep_sleep_start();
}
