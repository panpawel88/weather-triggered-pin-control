#include "config_print.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <string.h>

// Include config.h for WiFi credentials
#if __has_include("config.h")
    #include "config.h"
#endif

static const char *TAG = "CONFIG_PRINT";

void print_hardware_config(void) {
    ESP_LOGI(TAG, "=== Hardware Pin Configuration ===");
    ESP_LOGI(TAG, "Control GPIO Pin: %d", CONFIG_WEATHER_CONTROL_GPIO_PIN);
    ESP_LOGI(TAG, "LED Pin 1: %d", CONFIG_WEATHER_CONTROL_LED_PIN_1);
    ESP_LOGI(TAG, "LED Pin 2: %d", CONFIG_WEATHER_CONTROL_LED_PIN_2);
    ESP_LOGI(TAG, "LED Pin 3: %d", CONFIG_WEATHER_CONTROL_LED_PIN_3);
    ESP_LOGI(TAG, "LED Pin 4: %d", CONFIG_WEATHER_CONTROL_LED_PIN_4);
    ESP_LOGI(TAG, "LED Pin 5: %d", CONFIG_WEATHER_CONTROL_LED_PIN_5);
    ESP_LOGI(TAG, "I2C SDA Pin: %d", CONFIG_WEATHER_CONTROL_I2C_SDA);
    ESP_LOGI(TAG, "I2C SCL Pin: %d", CONFIG_WEATHER_CONTROL_I2C_SCL);
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
    ESP_LOGI(TAG, "Latitude: %s (from menuconfig default)", CONFIG_WEATHER_DEFAULT_LATITUDE);
#endif

#ifdef LONGITUDE
    ESP_LOGI(TAG, "Longitude: %.6f (from config.h override)", (double)LONGITUDE);
#else
    ESP_LOGI(TAG, "Longitude: %s (from menuconfig default)", CONFIG_WEATHER_DEFAULT_LONGITUDE);
#endif
}

void print_weather_schedule(void) {
    ESP_LOGI(TAG, "=== Weather Check Schedule ===");
    ESP_LOGI(TAG, "Weather check hour: %d:00 (24h format)", CONFIG_WEATHER_CHECK_HOUR);
}

void print_cloudcover_ranges(void) {
    ESP_LOGI(TAG, "=== Cloud Cover Ranges Configuration ===");
    ESP_LOGI(TAG, "Range 1: [%d%%, %d%%) -> Pin off at %d:00",
             CONFIG_WEATHER_RANGE_1_MIN,
             CONFIG_WEATHER_RANGE_1_MAX,
             CONFIG_WEATHER_RANGE_1_HOUR);
    ESP_LOGI(TAG, "Range 2: [%d%%, %d%%) -> Pin off at %d:00",
             CONFIG_WEATHER_RANGE_2_MIN,
             CONFIG_WEATHER_RANGE_2_MAX,
             CONFIG_WEATHER_RANGE_2_HOUR);
    ESP_LOGI(TAG, "Range 3: [%d%%, %d%%) -> Pin off at %d:00",
             CONFIG_WEATHER_RANGE_3_MIN,
             CONFIG_WEATHER_RANGE_3_MAX,
             CONFIG_WEATHER_RANGE_3_HOUR);
    ESP_LOGI(TAG, "Range 4: [%d%%, %d%%) -> Pin off at %d:00",
             CONFIG_WEATHER_RANGE_4_MIN,
             CONFIG_WEATHER_RANGE_4_MAX,
             CONFIG_WEATHER_RANGE_4_HOUR);
    ESP_LOGI(TAG, "Range 5: [%d%%, %d%%) -> Pin off at %d:00",
             CONFIG_WEATHER_RANGE_5_MIN,
             CONFIG_WEATHER_RANGE_5_MAX,
             CONFIG_WEATHER_RANGE_5_HOUR);
    ESP_LOGI(TAG, "Range 6: [%d%%, %d%%) -> Pin off at %d:00",
             CONFIG_WEATHER_RANGE_6_MIN,
             CONFIG_WEATHER_RANGE_6_MAX,
             CONFIG_WEATHER_RANGE_6_HOUR);
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
