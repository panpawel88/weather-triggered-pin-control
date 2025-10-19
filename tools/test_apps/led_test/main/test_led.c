#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_control.h"
#include "hardware_config.h"

static const char *TAG = "LED_TEST";

#define NUM_LEDS 5

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  LED Test Application");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // LED pins from hardware_config.h (same as main application)
    const gpio_num_t led_pins[NUM_LEDS] = {
        HW_LED_PIN_1,
        HW_LED_PIN_2,
        HW_LED_PIN_3,
        HW_LED_PIN_4,
        HW_LED_PIN_5
    };

    ESP_LOGI(TAG, "LED pins: %d, %d, %d, %d, %d",
             led_pins[0], led_pins[1], led_pins[2], led_pins[3], led_pins[4]);

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
