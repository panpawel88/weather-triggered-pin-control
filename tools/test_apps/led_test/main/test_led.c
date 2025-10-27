#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_gpio.h"
#include "hardware_config.h"

static const char *TAG = "LED_TEST";

#define NUM_LEDS HW_NUM_LEDS

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  LED Test Application");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // LED pins from hardware_config.h (same as main application)
    const gpio_num_t led_pins[NUM_LEDS] = HW_LED_PINS;

    ESP_LOGI(TAG, "Testing %d LEDs", NUM_LEDS);
    for (int i = 0; i < NUM_LEDS; i++) {
        ESP_LOGI(TAG, "  LED %d: GPIO %d", i + 1, led_pins[i]);
    }

    // Initialize all LEDs (turn them off)
    ESP_LOGI(TAG, "Initializing LEDs...");
    init_leds(led_pins, NUM_LEDS);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Starting LED test sequence...");
    ESP_LOGI(TAG, "LEDs will flash in sequence: 1 -> 2 -> 3 -> 4 -> 5 -> (repeat)");
    ESP_LOGI(TAG, "Press Ctrl+C to stop");
    ESP_LOGI(TAG, "");

    int iteration = 0;
    while (1) {
        iteration++;
        ESP_LOGI(TAG, "--- Iteration %d ---", iteration);

        for (int i = 0; i < NUM_LEDS; i++) {
            ESP_LOGI(TAG, "LED %d ON (GPIO %d)", i + 1, led_pins[i]);
            set_led(led_pins[i], true);
            vTaskDelay(500 / portTICK_PERIOD_MS);

            ESP_LOGI(TAG, "LED %d OFF (GPIO %d)", i + 1, led_pins[i]);
            set_led(led_pins[i], false);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }

        ESP_LOGI(TAG, "");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
