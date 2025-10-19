#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "driver/rmt_tx.h"

static const char *TAG = "RGB_LED_TEST";

// ============================================================================
// Configuration: RGB LED GPIO Pin
// ============================================================================
// Change this if the LED doesn't work with the default pin
// Common ESP32-S3 RGB LED pins: 48 (most common), 38, 8
#ifndef RGB_LED_GPIO
#define RGB_LED_GPIO 48
#endif

// LED strip configuration
#define LED_STRIP_LED_COUNT 1  // Usually just 1 RGB LED on dev boards
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)  // 10MHz resolution

// Rainbow effect parameters
#define RAINBOW_HUE_STEP 1      // How fast to cycle through colors (1-360)
#define RAINBOW_DELAY_MS 20     // Delay between color updates (ms)
#define BRIGHTNESS 50           // LED brightness (0-100)

// ============================================================================
// HSV to RGB Conversion
// ============================================================================

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

/**
 * Convert HSV to RGB color
 * H: 0-360 (hue)
 * S: 0-100 (saturation)
 * V: 0-100 (value/brightness)
 */
rgb_t hsv_to_rgb(float h, float s, float v) {
    rgb_t rgb = {0, 0, 0};

    s = s / 100.0f;
    v = v / 100.0f;

    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r, g, b;

    if (h < 60) {
        r = c; g = x; b = 0;
    } else if (h < 120) {
        r = x; g = c; b = 0;
    } else if (h < 180) {
        r = 0; g = c; b = x;
    } else if (h < 240) {
        r = 0; g = x; b = c;
    } else if (h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }

    rgb.r = (uint8_t)((r + m) * 255.0f);
    rgb.g = (uint8_t)((g + m) * 255.0f);
    rgb.b = (uint8_t)((b + m) * 255.0f);

    return rgb;
}

// ============================================================================
// Main Application
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32-S3 RGB LED Test Application");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Testing built-in WS2812/NeoPixel RGB LED");
    ESP_LOGI(TAG, "GPIO Pin: %d", RGB_LED_GPIO);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Common ESP32-S3 RGB LED pins:");
    ESP_LOGI(TAG, "  - GPIO 48 (ESP32-S3-DevKitC-1, most common)");
    ESP_LOGI(TAG, "  - GPIO 38 (some variants)");
    ESP_LOGI(TAG, "  - GPIO 8 (other variants)");
    ESP_LOGI(TAG, "");

    // Configure LED strip
    ESP_LOGI(TAG, "Configuring RMT for LED strip...");

    led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_LED_GPIO,
        .max_leds = LED_STRIP_LED_COUNT,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,  // WS2812 uses GRB format
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        .mem_block_symbols = 0,  // Set to 0 for auto-config (ESP32-S3 requires 48, not 64)
        .flags.with_dma = false,
    };

    led_strip_handle_t led_strip;
    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED strip! Error: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "Troubleshooting:");
        ESP_LOGE(TAG, "1. Check if GPIO %d is the correct pin for your board", RGB_LED_GPIO);
        ESP_LOGE(TAG, "2. Try changing RGB_LED_GPIO to 38 or 8");
        ESP_LOGE(TAG, "3. Check if your board has an RGB LED (look for WS2812 marking)");
        ESP_LOGE(TAG, "");
        return;
    }

    ESP_LOGI(TAG, "LED strip initialized successfully!");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Starting rainbow effect...");
    ESP_LOGI(TAG, "If you don't see the LED change colors, try a different GPIO pin.");
    ESP_LOGI(TAG, "Press Ctrl+C to stop");
    ESP_LOGI(TAG, "");

    // Clear LED first
    led_strip_clear(led_strip);

    // Rainbow effect loop
    float hue = 0;
    int color_cycle = 0;

    while (1) {
        // Convert HSV to RGB (full saturation, configured brightness)
        rgb_t color = hsv_to_rgb(hue, 100, BRIGHTNESS);

        // Set LED color
        led_strip_set_pixel(led_strip, 0, color.r, color.g, color.b);
        led_strip_refresh(led_strip);

        // Log color every 30 degrees (12 times per full cycle)
        if ((int)hue % 30 == 0 && hue > 0) {
            ESP_LOGI(TAG, "Hue: %.0f° | RGB: (%d, %d, %d) | Cycle: %d",
                     hue, color.r, color.g, color.b, color_cycle / 12);
        }

        // Increment hue
        hue += RAINBOW_HUE_STEP;
        if (hue >= 360.0f) {
            hue = 0;
            color_cycle++;
        }

        vTaskDelay(pdMS_TO_TICKS(RAINBOW_DELAY_MS));
    }
}
