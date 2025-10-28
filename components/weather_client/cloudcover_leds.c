#include "cloudcover_leds.h"
#include "led_gpio.h"
#include "hardware_config.h"
#include "esp_log.h"

static const char *TAG = "CLOUDCOVER_LEDS";

int led_count_from_cloudcover(float cloudcover) {
    // Edge case: no ranges defined
    if (HW_NUM_CLOUDCOVER_RANGES == 0) {
        ESP_LOGW(TAG, "No cloudcover ranges defined, returning 0 LEDs");
        return 0;
    }

    // Clamp cloudcover to valid range [0, 100]
    if (cloudcover < 0.0f) {
        ESP_LOGW(TAG, "Cloudcover %.1f%% below 0, clamping to 0%%", cloudcover);
        cloudcover = 0.0f;
    }
    if (cloudcover > 100.0f) {
        ESP_LOGW(TAG, "Cloudcover %.1f%% above 100, clamping to 100%%", cloudcover);
        cloudcover = 100.0f;
    }

    // Find which range the cloudcover falls into
    int range_index = -1;
    for (int i = 0; i < HW_NUM_CLOUDCOVER_RANGES; i++) {
        if (cloudcover >= HW_CLOUDCOVER_RANGES[i].min_cloudcover &&
            cloudcover < HW_CLOUDCOVER_RANGES[i].max_cloudcover) {
            range_index = i;
            break;
        }
    }

    // Edge case: cloudcover exactly at max boundary (e.g., exactly 100%)
    if (range_index == -1 && cloudcover == 100.0f) {
        range_index = HW_NUM_CLOUDCOVER_RANGES - 1;
    }

    if (range_index == -1) {
        ESP_LOGE(TAG, "Cloudcover %.1f%% does not fall into any defined range", cloudcover);
        return 0;
    }

    // Calculate LED count using proportional mapping
    // Invert range index: clearest sky (range 0) = most LEDs
    int inverted_index = (HW_NUM_CLOUDCOVER_RANGES - 1) - range_index;

    // Special case: only one range
    if (HW_NUM_CLOUDCOVER_RANGES == 1) {
        return HW_NUM_LEDS;
    }

    // Proportional mapping: led_count = (inverted_index * num_leds) / (num_ranges - 1)
    int led_count = (inverted_index * HW_NUM_LEDS) / (HW_NUM_CLOUDCOVER_RANGES - 1);

    ESP_LOGD(TAG, "Cloudcover %.1f%% -> range %d (%.1f-%.1f%%) -> %d LEDs",
             cloudcover, range_index,
             HW_CLOUDCOVER_RANGES[range_index].min_cloudcover,
             HW_CLOUDCOVER_RANGES[range_index].max_cloudcover,
             led_count);

    return led_count;
}

void control_leds(const gpio_num_t *led_pins, int num_leds, bool mainPinActive, float cloudcover) {
    if (!led_pins) {
        ESP_LOGE(TAG, "NULL led_pins pointer");
        return;
    }

    int activeLEDs = 0;
    if (mainPinActive) {
        activeLEDs = led_count_from_cloudcover(cloudcover);
        if (activeLEDs > num_leds) {
            activeLEDs = num_leds;
        }
    }

    ESP_LOGI(TAG, "LED control: main_pin=%s, cloudcover=%.1f%%, active_leds=%d",
             mainPinActive ? "ON" : "OFF", cloudcover, activeLEDs);

    // Set LED states and configure GPIO pins
    for (int i = 0; i < num_leds; i++) {
        bool ledOn = (i < activeLEDs);
        set_rtc_gpio_output(led_pins[i], ledOn ? 0 : 1);  // Active-low: 0=ON, 1=OFF
    }
}
