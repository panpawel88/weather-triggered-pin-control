#include "led_gpio.h"
#include "driver/rtc_io.h"
#include "esp_log.h"

static const char *TAG = "LED_GPIO";

void set_rtc_gpio_output(gpio_num_t pin, int level) {
    rtc_gpio_hold_dis(pin);  // Disable hold to allow state change
    rtc_gpio_init(pin);
    rtc_gpio_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(pin, level);
    rtc_gpio_hold_en(pin);  // Maintain state during sleep
}

void set_led(gpio_num_t led_pin, bool on) {
    set_rtc_gpio_output(led_pin, on ? 0 : 1);  // Active-low: 0=ON, 1=OFF
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
