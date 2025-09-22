#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "HELLO_WORLD";

void app_main(void)
{
    ESP_LOGI(TAG, "Hello World!");
    ESP_LOGI(TAG, "This is ESP32 chip with %d CPU core(s), WiFi%s%s, ",
             CONFIG_FREERTOS_NUMBER_OF_CORES,
             (esp_get_chip_features() & CHIP_FEATURE_BT) ? "/BT" : "",
             (esp_get_chip_features() & CHIP_FEATURE_BLE) ? "/BLE" : "");

    ESP_LOGI(TAG, "Silicon revision %d, ", esp_get_chip_revision());

    ESP_LOGI(TAG, "%dMB %s flash", spi_flash_get_chip_size() / (1024 * 1024),
             (esp_get_chip_features() & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(TAG, "Minimum free heap size: %d bytes", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) {
        ESP_LOGI(TAG, "Restarting in %d seconds...", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Restarting now.");
    fflush(stdout);
    esp_restart();
}