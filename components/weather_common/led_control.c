#include "led_control.h"
#include "driver/rtc_io.h"
#include "esp_log.h"

static const char *TAG = "LED_CONTROL";

int led_count_from_cloudcover(float cloudcover) {
    if (cloudcover >= 50.0f) return 0;      // Very cloudy (50-100%) -> 0 LEDs
    else if (cloudcover >= 40.0f) return 1; // Cloudy (40-49%) -> 1 LED
    else if (cloudcover >= 30.0f) return 2; // Partly cloudy (30-39%) -> 2 LEDs
    else if (cloudcover >= 20.0f) return 3; // Mostly clear (20-29%) -> 3 LEDs
    else if (cloudcover >= 10.0f) return 4; // Clear (10-19%) -> 4 LEDs
    else return 5;                           // Very clear (0-9%) -> 5 LEDs
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

        rtc_gpio_init(led_pins[i]);
        rtc_gpio_set_direction(led_pins[i], RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level(led_pins[i], ledOn ? 0 : 1);  // Active-low: 0=ON, 1=OFF
        rtc_gpio_hold_en(led_pins[i]);  // Maintain state during sleep
    }
}

void set_led(gpio_num_t led_pin, bool on) {
    rtc_gpio_init(led_pin);
    rtc_gpio_set_direction(led_pin, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(led_pin, on ? 0 : 1);  // Active-low: 0=ON, 1=OFF
    rtc_gpio_hold_en(led_pin);
}

void init_leds(const gpio_num_t *led_pins, int num_leds) {
    if (!led_pins) {
        ESP_LOGE(TAG, "NULL led_pins pointer");
        return;
    }

    ESP_LOGI(TAG, "Initializing %d LEDs", num_leds);
    for (int i = 0; i < num_leds; i++) {
        set_led(led_pins[i], false);  // Turn all LEDs off
    }
}
