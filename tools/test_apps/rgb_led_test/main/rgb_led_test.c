#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "rgb_led_control.h"
#include "hardware_config.h"

static const char *TAG = "RGB_LED_TEST";

// RTC memory variables persist during deep sleep
RTC_DATA_ATTR bool rgb_led_initialized = false;
RTC_DATA_ATTR int test_cycle = 0;

#define SLEEP_DURATION_SEC 5

void app_main(void) {
    test_cycle++;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  RGB LED Deep Sleep Test - Cycle %d", test_cycle);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Initialize RGB LED (RMT peripheral needs setup after deep sleep)
    // Clear LED only on first boot (!rgb_led_initialized = true)
    // Preserve LED state on wakeups (!rgb_led_initialized = false)
    if (!rgb_led_initialized) {
        ESP_LOGI(TAG, "First boot - initializing RGB LED (will clear)");
    } else {
        ESP_LOGI(TAG, "Woke from deep sleep - reinitializing RGB LED (preserving state)");
    }

    rgb_led_initialized = (rgb_led_init(!rgb_led_initialized) == ESP_OK);
    if (!rgb_led_initialized) {
        ESP_LOGE(TAG, "RGB LED initialization failed!");
        ESP_LOGI(TAG, "Test cannot continue. Check GPIO configuration in hardware_config.h");
        return;
    }
    ESP_LOGI(TAG, "RGB LED initialized successfully");

    ESP_LOGI(TAG, "");

    // Test sequence based on cycle number
    int step = (test_cycle - 1) % 3; // 0, 1, 2, 0, 1, 2, ...

    switch (step) {
        case 0:
            ESP_LOGI(TAG, "Step 1/3: Turning RGB LED ON (red)");
            if (rgb_led_set_state(true) == ESP_OK) {
                ESP_LOGI(TAG, "RGB LED is now ON");
            } else {
                ESP_LOGE(TAG, "Failed to turn RGB LED ON");
            }
            break;

        case 1:
            ESP_LOGI(TAG, "Step 2/3: Turning RGB LED OFF");
            if (rgb_led_set_state(false) == ESP_OK) {
                ESP_LOGI(TAG, "RGB LED is now OFF");
            } else {
                ESP_LOGE(TAG, "Failed to turn RGB LED OFF");
            }
            break;

        case 2:
            ESP_LOGI(TAG, "Step 3/3: Turning RGB LED ON again (after sleep)");
            if (rgb_led_set_state(true) == ESP_OK) {
                ESP_LOGI(TAG, "RGB LED is now ON");
                ESP_LOGI(TAG, "");
                ESP_LOGI(TAG, "*** Test cycle complete! ***");
                ESP_LOGI(TAG, "The LED should have stayed OFF during this wake cycle");
                ESP_LOGI(TAG, "and turned ON just now without any flicker.");
            } else {
                ESP_LOGE(TAG, "Failed to turn RGB LED ON");
            }
            break;
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Entering deep sleep for %d seconds...", SLEEP_DURATION_SEC);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Small delay to ensure log is printed
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Enter deep sleep
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_SEC * 1000000ULL);
    esp_deep_sleep_start();
}
