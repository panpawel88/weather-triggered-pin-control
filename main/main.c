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
#include "timezone_helper.h"
#include "led_control.h"
#include "weather_fetch.h"
#include "wifi_helper.h"
#include "config_print.h"
#include "hardware_config.h"
#include "remote_logging.h"

// WiFi credentials and location override come from config.h (in weather_common component)
// This file is gitignored and must be created from config.h.example
#ifndef __has_include
    #error "Compiler does not support __has_include"
#endif

#if !__has_include("config.h")
    #error "config.h not found! Please copy components/weather_common/include/config.h.example to components/weather_common/include/config.h"
#endif
#include "config.h"

static const char *TAG = "WEATHER_CONTROL";

// Location configuration: Use config.h overrides if defined, otherwise use hardware defaults
#ifndef LATITUDE
    #define LATITUDE HW_DEFAULT_LATITUDE
#endif
#ifndef LONGITUDE
    #define LONGITUDE HW_DEFAULT_LONGITUDE
#endif

// Hardware configuration from hardware_config.h (single source of truth)
#define GPIO_CONTROL_PIN HW_GPIO_CONTROL_PIN
#define LED_PIN_1 HW_LED_PIN_1
#define LED_PIN_2 HW_LED_PIN_2
#define LED_PIN_3 HW_LED_PIN_3
#define LED_PIN_4 HW_LED_PIN_4
#define LED_PIN_5 HW_LED_PIN_5
#define NUM_LEDS HW_NUM_LEDS

#define I2C_MASTER_SDA_IO HW_I2C_SDA_PIN
#define I2C_MASTER_SCL_IO HW_I2C_SCL_PIN

#define WEATHER_CHECK_HOUR HW_WEATHER_CHECK_HOUR

// RTC memory variables persist during deep sleep
RTC_DATA_ATTR int pinOffHour = 17;  // Default to 5 PM if no weather data
RTC_DATA_ATTR bool weatherFetched = false;
RTC_DATA_ATTR float currentCloudCover = 75.0f;  // Default to cloudy (0 LEDs)
RTC_DATA_ATTR bool ledStates[NUM_LEDS] = {false, false, false, false, false};
RTC_DATA_ATTR bool lastPinState = false;  // Track previous pin state for edge detection

// Find appropriate pin-off hour based on cloudcover percentage
static int getPinOffHourFromCloudcover(float cloudcover) {
    for (int i = 0; i < HW_NUM_CLOUDCOVER_RANGES; i++) {
        if (cloudcover >= HW_CLOUDCOVER_RANGES[i].min_cloudcover &&
            cloudcover < HW_CLOUDCOVER_RANGES[i].max_cloudcover) {
            ESP_LOGI(TAG, "Cloudcover %.1f%% matches range [%.1f, %.1f) -> pin off at %d:00",
                     cloudcover,
                     HW_CLOUDCOVER_RANGES[i].min_cloudcover,
                     HW_CLOUDCOVER_RANGES[i].max_cloudcover,
                     HW_CLOUDCOVER_RANGES[i].pin_high_until_hour);
            return HW_CLOUDCOVER_RANGES[i].pin_high_until_hour;
        }
    }
    // Default fallback (shouldn't happen with proper ranges)
    ESP_LOGW(TAG, "No range found for cloudcover %.1f%%, using default 5 PM", cloudcover);
    return 17;
}

void fetchWeatherForecast(void) {
    ESP_LOGI(TAG, "Starting weather fetch");

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
}

void controlGPIO(const datetime_t *local_time) {
    static const gpio_num_t led_pins[NUM_LEDS] = {
        LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4, LED_PIN_5
    };

    int hour = local_time->hour;
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
    set_rtc_gpio_output(GPIO_CONTROL_PIN, activate ? 1 : 0);

    // Control LEDs - only show if weather has been fetched AND main pin is active
    bool showLEDs = weatherFetched && activate;
    control_leds(led_pins, NUM_LEDS, showLEDs, currentCloudCover);
}

void app_main(void) {
    ESP_LOGI(TAG, "Weather Triggered Pin Control starting");

    // Initialize timezone for DST support
    if (timezone_init() != ESP_OK) {
        ESP_LOGE(TAG, "Timezone initialization failed");
    }

    // Initialize I2C for RTC first (needed for timestamps in remote logging)
    if (rtc_i2c_init(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO) != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed, restarting");
        esp_restart();
    }

    // Initialize remote logging (buffers logs for sending to HTTP server)
    if (remote_logging_init() == ESP_OK) {
        ESP_LOGI(TAG, "Remote logging initialized");
    }

    // Log cloudcover configuration ranges
    ESP_LOGI(TAG, "Cloudcover ranges configuration:");
    for (int i = 0; i < HW_NUM_CLOUDCOVER_RANGES; i++) {
        ESP_LOGI(TAG, "  Range %d: [%.1f%%, %.1f%%) -> pin off at %d:00",
                 i + 1,
                 HW_CLOUDCOVER_RANGES[i].min_cloudcover,
                 HW_CLOUDCOVER_RANGES[i].max_cloudcover,
                 HW_CLOUDCOVER_RANGES[i].pin_high_until_hour);
    }

    // Read UTC time from RTC
    datetime_t utc_time;
    if (rtc_read_time(&utc_time) != ESP_OK) {
        ESP_LOGE(TAG, "RTC read failed, restarting");
        esp_restart();
    }

    // Convert UTC to local time (CET/CEST with automatic DST)
    datetime_t local_time;
    if (utc_to_local(&utc_time, &local_time) != ESP_OK) {
        ESP_LOGE(TAG, "Timezone conversion failed, restarting");
        esp_restart();
    }

    // Get timezone info for display
    char tz_abbr[8];
    get_timezone_abbr(&utc_time, tz_abbr);

    ESP_LOGI(TAG, "UTC time:   %04d-%02d-%02d %02d:%02d:%02d",
             utc_time.year, utc_time.month, utc_time.day,
             utc_time.hour, utc_time.minute, utc_time.second);
    ESP_LOGI(TAG, "Local time: %04d-%02d-%02d %02d:%02d:%02d %s",
             local_time.year, local_time.month, local_time.day,
             local_time.hour, local_time.minute, local_time.second, tz_abbr);
    ESP_LOGI(TAG, "Current pin-off hour setting: %d:00 (weather fetched: %s)",
             pinOffHour, weatherFetched ? "yes" : "no");
    ESP_LOGI(TAG, "Current cloud cover: %.1f%% -> %d LEDs active",
             currentCloudCover, led_count_from_cloudcover(currentCloudCover));

    // Control GPIO based on weather and local time
    controlGPIO(&local_time);

    // Enable WiFi every hour for remote logging (and weather fetch at 4 PM)
    ESP_LOGI(TAG, "Initializing WiFi");
    if (wifi_init() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed");
    } else {
        // Wait for WiFi connection
        if (wifi_wait_connected(20, 500) != ESP_OK) {
            ESP_LOGE(TAG, "WiFi connection failed");
        } else {
            // Fetch weather at 4 PM local time
            if (local_time.hour == WEATHER_CHECK_HOUR && !weatherFetched) {
                fetchWeatherForecast();
                weatherFetched = true;
            }

            // Flush buffered logs to remote server while WiFi is active
            int buffered = remote_logging_get_buffered_count();
            int dropped = remote_logging_get_dropped_count();
            if (buffered > 0 || dropped > 0) {
                ESP_LOGI(TAG, "Flushing %d buffered logs (dropped: %d) to remote server", buffered, dropped);
                if (remote_logging_flush() == ESP_OK) {
                    ESP_LOGI(TAG, "Remote log flush successful");
                } else {
                    ESP_LOGW(TAG, "Remote log flush failed, logs will be retried next time");
                }
            }
        }

        // Shutdown WiFi to save power
        wifi_shutdown();
    }

    ESP_LOGI(TAG, "Going to deep sleep for 1 hour");

    // Deep sleep for 1 hour
    esp_sleep_enable_timer_wakeup(3600 * 1000000ULL);
    esp_deep_sleep_start();
}
