#include "config_print.h"
#include "hardware_config.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <string.h>

// Include config.h for WiFi credentials (if available)
#if __has_include("config.h")
    #include "config.h"
#endif

static const char *TAG = "CONFIG_PRINT";

void print_hardware_config(void) {
    ESP_LOGI(TAG, "=== Hardware Pin Configuration ===");
    ESP_LOGI(TAG, "Control GPIO Pin: %d", HW_GPIO_CONTROL_PIN);
    ESP_LOGI(TAG, "LED Pin 1: %d", HW_LED_PIN_1);
    ESP_LOGI(TAG, "LED Pin 2: %d", HW_LED_PIN_2);
    ESP_LOGI(TAG, "LED Pin 3: %d", HW_LED_PIN_3);
    ESP_LOGI(TAG, "LED Pin 4: %d", HW_LED_PIN_4);
    ESP_LOGI(TAG, "LED Pin 5: %d", HW_LED_PIN_5);
    ESP_LOGI(TAG, "I2C SDA Pin: %d", HW_I2C_SDA_PIN);
    ESP_LOGI(TAG, "I2C SCL Pin: %d", HW_I2C_SCL_PIN);
}

void print_wifi_config(void) {
    ESP_LOGI(TAG, "=== WiFi Configuration ===");
#ifdef WIFI_SSID
    ESP_LOGI(TAG, "WiFi SSID: %s", WIFI_SSID);
#ifdef WIFI_PASSWORD
    size_t pwd_len = strlen(WIFI_PASSWORD);
    ESP_LOGI(TAG, "WiFi Password: [%zu characters, hidden]", pwd_len);
#else
    ESP_LOGI(TAG, "WiFi Password: [NOT SET]");
#endif
#else
    ESP_LOGI(TAG, "WiFi configuration not found in config.h");
#endif
}

void print_location_config(void) {
    ESP_LOGI(TAG, "=== Location Configuration ===");

#ifdef LATITUDE
    ESP_LOGI(TAG, "Latitude: %.6f (from config.h override)", (double)LATITUDE);
#else
    ESP_LOGI(TAG, "Latitude: %.6f (from hardware_config.h)", (double)HW_DEFAULT_LATITUDE);
#endif

#ifdef LONGITUDE
    ESP_LOGI(TAG, "Longitude: %.6f (from config.h override)", (double)LONGITUDE);
#else
    ESP_LOGI(TAG, "Longitude: %.6f (from hardware_config.h)", (double)HW_DEFAULT_LONGITUDE);
#endif
}

void print_weather_schedule(void) {
    ESP_LOGI(TAG, "=== Weather Check Schedule ===");
    ESP_LOGI(TAG, "Weather check hour: %d:00 (24h format)", HW_WEATHER_CHECK_HOUR);
}

void print_cloudcover_ranges(void) {
    ESP_LOGI(TAG, "=== Cloud Cover Ranges Configuration ===");
    for (int i = 0; i < HW_NUM_CLOUDCOVER_RANGES; i++) {
        ESP_LOGI(TAG, "Range %d: [%.0f%%, %.0f%%) -> Pin off at %d:00",
                 i + 1,
                 HW_CLOUDCOVER_RANGES[i].min_cloudcover,
                 HW_CLOUDCOVER_RANGES[i].max_cloudcover,
                 HW_CLOUDCOVER_RANGES[i].pin_high_until_hour);
    }
}

void print_all_config(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Weather Control Configuration");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    print_hardware_config();
    ESP_LOGI(TAG, "");

    print_wifi_config();
    ESP_LOGI(TAG, "");

    print_location_config();
    ESP_LOGI(TAG, "");

    print_weather_schedule();
    ESP_LOGI(TAG, "");

    print_cloudcover_ranges();
    ESP_LOGI(TAG, "");

    ESP_LOGI(TAG, "========================================");
}
