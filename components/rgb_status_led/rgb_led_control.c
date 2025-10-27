#include "rgb_led_control.h"

// Only compile this code if RGB LED feature is enabled
#if HW_RGB_LED_ENABLED

#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "RGB_LED";

// LED strip handle (NULL if not initialized)
static led_strip_handle_t led_strip = NULL;

// LED strip configuration
#define LED_STRIP_LED_COUNT 1  // Single RGB LED on dev boards
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)  // 10MHz resolution

esp_err_t rgb_led_init(bool clear_strip) {
    if (led_strip != NULL) {
        ESP_LOGW(TAG, "RGB LED already initialized, reinitializing...");
        // Clean up existing handle before reinitializing
        led_strip_del(led_strip);
        led_strip = NULL;
    }

    ESP_LOGI(TAG, "Initializing RGB LED on GPIO %d", HW_RGB_LED_GPIO);

    // Configure LED strip
    led_strip_config_t strip_config = {
        .strip_gpio_num = HW_RGB_LED_GPIO,
        .max_leds = LED_STRIP_LED_COUNT,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,  // WS2812 uses GRB format
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        .mem_block_symbols = 0,  // Auto-config
        .flags.with_dma = false,
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RGB LED strip: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check if GPIO %d is correct for your board", HW_RGB_LED_GPIO);
        led_strip = NULL;
        return ESP_FAIL;
    }

    // Clear LED only if requested (first boot), otherwise preserve state (wakeup)
    if (clear_strip) {
        ESP_LOGI(TAG, "Clearing LED on init");
        led_strip_clear(led_strip);
    } else {
        ESP_LOGI(TAG, "Preserving LED state on init (wakeup)");
    }

    ESP_LOGI(TAG, "RGB LED initialized successfully (color: R=%d G=%d B=%d, brightness: %d%%)",
             HW_RGB_LED_COLOR_R, HW_RGB_LED_COLOR_G, HW_RGB_LED_COLOR_B, HW_RGB_LED_BRIGHTNESS);

    return ESP_OK;
}

esp_err_t rgb_led_set_state(bool on) {
    if (led_strip == NULL) {
        ESP_LOGE(TAG, "RGB LED not initialized");
        return ESP_FAIL;
    }

    if (on) {
        // Calculate RGB values with brightness applied
        // brightness is 0-100, convert to 0.0-1.0 multiplier
        float brightness_factor = HW_RGB_LED_BRIGHTNESS / 100.0f;
        uint8_t r = (uint8_t)(HW_RGB_LED_COLOR_R * brightness_factor);
        uint8_t g = (uint8_t)(HW_RGB_LED_COLOR_G * brightness_factor);
        uint8_t b = (uint8_t)(HW_RGB_LED_COLOR_B * brightness_factor);

        ESP_LOGI(TAG, "RGB LED ON (R=%d G=%d B=%d)", r, g, b);
        led_strip_set_pixel(led_strip, 0, r, g, b);
    } else {
        ESP_LOGI(TAG, "RGB LED OFF");
        led_strip_set_pixel(led_strip, 0, 0, 0, 0);
    }

    return led_strip_refresh(led_strip);
}

esp_err_t rgb_led_deinit(void) {
    if (led_strip != NULL) {
        ESP_LOGI(TAG, "Deinitializing RGB LED");
        led_strip_clear(led_strip);
        led_strip_del(led_strip);
        led_strip = NULL;
    }
    return ESP_OK;
}

#endif // HW_RGB_LED_ENABLED
