#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

// Include shared components
#include "rtc_helper.h"
#include "led_control.h"
#include "weather_fetch.h"
#include "config_print.h"

// Include local configuration (WiFi credentials, optional location overrides)
// This file is gitignored and must be created by copying config.h.example
#ifndef __has_include
    #error "Compiler does not support __has_include"
#endif

#if !__has_include("config.h")
    #error "config.h not found! Please copy main/config.h.example to main/config.h and configure it."
#endif
#include "config.h"

static const char *TAG = "WEATHER_CONTROL";

// Location configuration: Use config.h overrides if defined, otherwise use menuconfig defaults
#ifndef LATITUDE
    #define LATITUDE atof(CONFIG_WEATHER_DEFAULT_LATITUDE)
#endif
#ifndef LONGITUDE
    #define LONGITUDE atof(CONFIG_WEATHER_DEFAULT_LONGITUDE)
#endif

// Hardware pins from menuconfig
#define GPIO_CONTROL_PIN CONFIG_WEATHER_CONTROL_GPIO_PIN
#define LED_PIN_1 CONFIG_WEATHER_CONTROL_LED_PIN_1
#define LED_PIN_2 CONFIG_WEATHER_CONTROL_LED_PIN_2
#define LED_PIN_3 CONFIG_WEATHER_CONTROL_LED_PIN_3
#define LED_PIN_4 CONFIG_WEATHER_CONTROL_LED_PIN_4
#define LED_PIN_5 CONFIG_WEATHER_CONTROL_LED_PIN_5
#define NUM_LEDS 5

// I2C configuration for DS3231 RTC
#define I2C_MASTER_SCL_IO CONFIG_WEATHER_CONTROL_I2C_SCL
#define I2C_MASTER_SDA_IO CONFIG_WEATHER_CONTROL_I2C_SDA

// Weather check schedule from menuconfig
#define WEATHER_CHECK_HOUR CONFIG_WEATHER_CHECK_HOUR

// RTC memory variables persist during deep sleep
RTC_DATA_ATTR int pinOffHour = 17;  // Default to 5 PM if no weather data
RTC_DATA_ATTR bool weatherFetched = false;
RTC_DATA_ATTR float currentCloudCover = 75.0f;  // Default to cloudy (0 LEDs)
RTC_DATA_ATTR bool ledStates[NUM_LEDS] = {false, false, false, false, false};
RTC_DATA_ATTR bool lastPinState = false;  // Track previous pin state for edge detection

typedef struct {
    float min_cloudcover;    // Minimum cloudcover percentage (inclusive)
    float max_cloudcover;    // Maximum cloudcover percentage (exclusive)
    int pin_high_until_hour; // Hour when pin goes low (24h format)
} cloudcover_range_t;

// Configuration: Cloudcover ranges (from menuconfig)
#define NUM_CLOUDCOVER_RANGES 6
static const cloudcover_range_t CLOUDCOVER_RANGES[NUM_CLOUDCOVER_RANGES] = {
    {CONFIG_WEATHER_RANGE_1_MIN, CONFIG_WEATHER_RANGE_1_MAX, CONFIG_WEATHER_RANGE_1_HOUR},
    {CONFIG_WEATHER_RANGE_2_MIN, CONFIG_WEATHER_RANGE_2_MAX, CONFIG_WEATHER_RANGE_2_HOUR},
    {CONFIG_WEATHER_RANGE_3_MIN, CONFIG_WEATHER_RANGE_3_MAX, CONFIG_WEATHER_RANGE_3_HOUR},
    {CONFIG_WEATHER_RANGE_4_MIN, CONFIG_WEATHER_RANGE_4_MAX, CONFIG_WEATHER_RANGE_4_HOUR},
    {CONFIG_WEATHER_RANGE_5_MIN, CONFIG_WEATHER_RANGE_5_MAX, CONFIG_WEATHER_RANGE_5_HOUR},
    {CONFIG_WEATHER_RANGE_6_MIN, CONFIG_WEATHER_RANGE_6_MAX, CONFIG_WEATHER_RANGE_6_HOUR}
};

// Find appropriate pin-off hour based on cloudcover percentage
static int getPinOffHourFromCloudcover(float cloudcover) {
    for (int i = 0; i < NUM_CLOUDCOVER_RANGES; i++) {
        if (cloudcover >= CLOUDCOVER_RANGES[i].min_cloudcover &&
            cloudcover < CLOUDCOVER_RANGES[i].max_cloudcover) {
            ESP_LOGI(TAG, "Cloudcover %.1f%% matches range [%.1f, %.1f) -> pin off at %d:00",
                     cloudcover,
                     CLOUDCOVER_RANGES[i].min_cloudcover,
                     CLOUDCOVER_RANGES[i].max_cloudcover,
                     CLOUDCOVER_RANGES[i].pin_high_until_hour);
            return CLOUDCOVER_RANGES[i].pin_high_until_hour;
        }
    }
    // Default fallback (shouldn't happen with proper ranges)
    ESP_LOGW(TAG, "No range found for cloudcover %.1f%%, using default 5 PM", cloudcover);
    return 17;
}

void fetchWeatherForecast(void) {
    ESP_LOGI(TAG, "Starting weather fetch");

    if (weather_wifi_init() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed");
        return;
    }

    // Wait for WiFi connection
    if (weather_wifi_wait_connected(20, 500) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connection failed");
        weather_wifi_shutdown();
        return;
    }

    weather_data_t weather_data;
    if (fetch_weather_forecast(LATITUDE, LONGITUDE, &weather_data) == ESP_OK && weather_data.valid) {
        currentCloudCover = weather_data.tomorrow_cloudcover;
        pinOffHour = getPinOffHourFromCloudcover(weather_data.tomorrow_cloudcover);
        ESP_LOGI(TAG, "Tomorrow cloud cover: %.1f%% -> pin will turn off at %d:00, LEDs: %d",
                weather_data.tomorrow_cloudcover, pinOffHour,
                led_count_from_cloudcover(weather_data.tomorrow_cloudcover));
    } else {
        ESP_LOGE(TAG, "Failed to fetch valid weather data");
    }

    weather_wifi_shutdown();
}

void controlGPIO(datetime_t *now) {
    static const gpio_num_t led_pins[NUM_LEDS] = {
        LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4, LED_PIN_5
    };

    int hour = now->hour;
    bool activate = (hour >= 9 && hour < pinOffHour);

    // Detect pin turning off (transition from ON to OFF)
    if (lastPinState && !activate) {
        weatherFetched = false;  // Reset when pin turns off
        ESP_LOGI(TAG, "Main pin turning off, resetting weatherFetched flag");
    }
    lastPinState = activate;

    ESP_LOGI(TAG, "GPIO control: hour=%d, pin_off_hour=%d, activate=%s, weather_fetched=%s",
             hour, pinOffHour, activate ? "yes" : "no", weatherFetched ? "yes" : "no");

    // Control main GPIO pin
    rtc_gpio_init(GPIO_CONTROL_PIN);
    rtc_gpio_set_direction(GPIO_CONTROL_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(GPIO_CONTROL_PIN, activate ? 1 : 0);
    rtc_gpio_hold_en(GPIO_CONTROL_PIN);  // Maintain state during sleep

    // Control LEDs - only show if weather has been fetched AND main pin is active
    bool showLEDs = weatherFetched && activate;
    control_leds(led_pins, NUM_LEDS, showLEDs, currentCloudCover);
}

void app_main(void) {
    ESP_LOGI(TAG, "Weather Triggered Pin Control starting");

    // Log cloudcover configuration ranges
    ESP_LOGI(TAG, "Cloudcover ranges configuration:");
    for (int i = 0; i < NUM_CLOUDCOVER_RANGES; i++) {
        ESP_LOGI(TAG, "  Range %d: [%.1f%%, %.1f%%) -> pin off at %d:00",
                 i + 1,
                 CLOUDCOVER_RANGES[i].min_cloudcover,
                 CLOUDCOVER_RANGES[i].max_cloudcover,
                 CLOUDCOVER_RANGES[i].pin_high_until_hour);
    }

    // Initialize I2C for RTC
    if (rtc_i2c_init(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO) != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed, restarting");
        esp_restart();
    }

    datetime_t now;
    if (rtc_read_time(&now) != ESP_OK) {
        ESP_LOGE(TAG, "RTC read failed, restarting");
        esp_restart();
    }

    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
             now.year, now.month, now.day, now.hour, now.minute, now.second);
    ESP_LOGI(TAG, "Current pin-off hour setting: %d:00 (weather fetched: %s)",
             pinOffHour, weatherFetched ? "yes" : "no");
    ESP_LOGI(TAG, "Current cloud cover: %.1f%% -> %d LEDs active",
             currentCloudCover, led_count_from_cloudcover(currentCloudCover));

    // Fetch weather at 4 PM
    if (now.hour == WEATHER_CHECK_HOUR && !weatherFetched) {
        fetchWeatherForecast();
        weatherFetched = true;
    }

    // Control GPIO based on weather and time
    controlGPIO(&now);

    ESP_LOGI(TAG, "Going to deep sleep for 1 hour");

    // Deep sleep for 1 hour
    esp_sleep_enable_timer_wakeup(3600 * 1000000ULL);
    esp_deep_sleep_start();
}
