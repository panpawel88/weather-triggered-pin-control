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
#include "led_gpio.h"
#include "cloudcover_leds.h"
#include "rgb_led_control.h"
#include "weather_fetch.h"
#include "wifi_helper.h"
#include "config_print.h"
#include "hardware_config.h"
#include "remote_logging.h"

// WiFi credentials and location override come from config.h (in hardware_config component)
// This file is gitignored and must be created from config.h.example
#ifndef __has_include
    #error "Compiler does not support __has_include"
#endif

#if !__has_include("config.h")
    #error "config.h not found! Please copy components/hardware_config/include/config.h.example to components/hardware_config/include/config.h"
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
#define NUM_LEDS HW_NUM_LEDS

#define I2C_MASTER_SDA_IO HW_I2C_SDA_PIN
#define I2C_MASTER_SCL_IO HW_I2C_SCL_PIN

#define WEATHER_CHECK_HOUR HW_WEATHER_CHECK_HOUR

// RTC memory variables persist during deep sleep
RTC_DATA_ATTR int pin_off_hour = 17;  // Default to 5 PM if no weather data
RTC_DATA_ATTR bool weather_fetched = false;
RTC_DATA_ATTR float current_cloud_cover = 75.0f;  // Default to cloudy (0 LEDs)
RTC_DATA_ATTR bool last_pin_state = false;  // Track previous pin state for edge detection
RTC_DATA_ATTR bool rgb_led_initialized = false;  // Track RGB LED initialization state

// Find appropriate pin-off hour based on cloudcover percentage
static int get_pin_off_hour_from_cloudcover(float cloudcover) {
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

// Wait until the clock reaches HH:00:30 to compensate for deep sleep timer inaccuracy
static void wait_until_target_second(void) {
    datetime_t utc_time, local_time;

    // Read current time
    if (rtc_read_time(&utc_time) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read RTC time for synchronization");
        return;
    }

    if (utc_to_local(&utc_time, &local_time) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to convert time for synchronization");
        return;
    }

    ESP_LOGI(TAG, "Wake time: %02d:%02d:%02d",
             local_time.hour, local_time.minute, local_time.second);

    // Only wait if minute is 0 or >= 58 (within ~2 minutes of target hour)
    if (local_time.minute == 0 || local_time.minute >= 58) {
        // Calculate seconds until HH:00:30
        int target_second = 30;
        int seconds_to_wait = 0;

        if (local_time.minute >= 58) {
            // We're in the previous hour, need to wait until next hour at :00:30
            int seconds_in_current_minute = local_time.second;
            int seconds_to_next_hour = (60 - local_time.minute) * 60 - seconds_in_current_minute;
            seconds_to_wait = seconds_to_next_hour + target_second;
        } else if (local_time.minute == 0) {
            // We're in the target hour
            if (local_time.second < target_second) {
                // Wait until :00:30
                seconds_to_wait = target_second - local_time.second;
            } else {
                // Already past :00:30, proceed immediately
                ESP_LOGI(TAG, "Already past target time (%02d:%02d:%02d), proceeding immediately",
                         local_time.hour, local_time.minute, local_time.second);
                return;
            }
        }

        if (seconds_to_wait > 0) {
            ESP_LOGI(TAG, "Waiting %d seconds until %02d:00:30",
                     seconds_to_wait, (local_time.minute >= 58) ? (local_time.hour + 1) % 24 : local_time.hour);

            // Wait in small increments for better responsiveness
            while (seconds_to_wait > 0) {
                int delay_ms = (seconds_to_wait > 1) ? 1000 : (seconds_to_wait * 1000);
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
                seconds_to_wait--;
            }

            // Verify we reached the target time
            if (rtc_read_time(&utc_time) == ESP_OK && utc_to_local(&utc_time, &local_time) == ESP_OK) {
                ESP_LOGI(TAG, "Target time reached: %02d:%02d:%02d",
                         local_time.hour, local_time.minute, local_time.second);
            }
        }
    } else {
        ESP_LOGI(TAG, "Woke outside synchronization window (minute=%d), proceeding immediately",
                 local_time.minute);
    }
}

void fetch_weather_forecast_and_update(void) {
    ESP_LOGI(TAG, "Starting weather fetch");

    weather_data_t weather_data;
    if (fetch_weather_forecast(LATITUDE, LONGITUDE, &weather_data) == ESP_OK && weather_data.valid) {
        current_cloud_cover = weather_data.tomorrow_cloudcover;
        pin_off_hour = get_pin_off_hour_from_cloudcover(weather_data.tomorrow_cloudcover);
        ESP_LOGI(TAG, "Tomorrow cloud cover: %.1f%% -> pin will turn off at %d:00, LEDs: %d",
                weather_data.tomorrow_cloudcover, pin_off_hour,
                led_count_from_cloudcover(weather_data.tomorrow_cloudcover));
    } else {
        ESP_LOGE(TAG, "Failed to fetch valid weather data");
    }
}

void control_gpio(const datetime_t *local_time) {
    static const gpio_num_t led_pins[NUM_LEDS] = HW_LED_PINS;

    int hour = local_time->hour;
    bool activate = (hour >= 9 && hour < pin_off_hour);

    // Detect pin turning off (transition from ON to OFF)
    if (last_pin_state && !activate) {
        weather_fetched = false;  // Reset when pin turns off
        ESP_LOGI(TAG, "Main pin turning off, resetting weather_fetched flag");
    }
    last_pin_state = activate;

    ESP_LOGI(TAG, "GPIO control: hour=%d, pin_off_hour=%d, activate=%s, weather_fetched=%s",
             hour, pin_off_hour, activate ? "yes" : "no", weather_fetched ? "yes" : "no");

    // Control main GPIO pin
    set_rtc_gpio_output(GPIO_CONTROL_PIN, activate ? 1 : 0);

    // Control RGB LED to mirror GPIO control pin state
    rgb_led_set_state(activate);

    // Control LEDs - only show if weather has been fetched AND main pin is active
    bool show_leds = weather_fetched && activate;
    control_leds(led_pins, NUM_LEDS, show_leds, current_cloud_cover);
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

    // Initialize RGB LED (optional feature, configured in hardware_config.h)
    // Always initialize (RMT peripheral needs setup after deep sleep)
    // Clear LED only on first boot (!rgb_led_initialized = true)
    // Preserve LED state on wakeups (!rgb_led_initialized = false)
    rgb_led_initialized = (rgb_led_init(!rgb_led_initialized) == ESP_OK);
    if (rgb_led_initialized) {
        ESP_LOGI(TAG, "RGB LED initialized");
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
             pin_off_hour, weather_fetched ? "yes" : "no");
    ESP_LOGI(TAG, "Current cloud cover: %.1f%% -> %d LEDs active",
             current_cloud_cover, led_count_from_cloudcover(current_cloud_cover));

    // Wait until HH:00:30 to compensate for deep sleep timer inaccuracy
    wait_until_target_second();

    // Enable WiFi every hour for remote logging (and weather fetch at 4 PM)
    ESP_LOGI(TAG, "Initializing WiFi");
    bool wifi_connected = false;
    if (wifi_init() == ESP_OK) {
        if (wifi_wait_connected(20, 500) == ESP_OK) {
            wifi_connected = true;
        } else {
            ESP_LOGE(TAG, "WiFi connection failed");
        }
    } else {
        ESP_LOGE(TAG, "WiFi init failed");
    }

    // Fetch weather at 4 PM local time if WiFi is connected
    if (wifi_connected && local_time.hour == WEATHER_CHECK_HOUR && !weather_fetched) {
        fetch_weather_forecast_and_update();
        weather_fetched = true;
    }

    // Control GPIO and LEDs based on weather and local time
    control_gpio(&local_time);

    // Calculate sleep duration to wake at next full hour + 30 seconds
    int sleep_seconds = 3600; // Default 1 hour fallback
    datetime_t current_utc, current_local;
    if (rtc_read_time(&current_utc) == ESP_OK && utc_to_local(&current_utc, &current_local) == ESP_OK) {
        int seconds_into_hour = current_local.minute * 60 + current_local.second;
        int seconds_until_next_hour = 3600 - seconds_into_hour;
        sleep_seconds = seconds_until_next_hour + 30; // Add 30s buffer
        int next_hour = (current_local.hour + 1) % 24;

        ESP_LOGI(TAG, "Current time: %02d:%02d:%02d, sleeping for %d seconds until %02d:00:30",
                 current_local.hour, current_local.minute, current_local.second,
                 sleep_seconds, next_hour);
    } else {
        ESP_LOGE(TAG, "Failed to read time for sleep calculation, using default 1 hour");
    }

    // Flush buffered logs to remote server if WiFi is connected
    if (wifi_connected) {
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

    // Configure sleep timer and enter deep sleep
    esp_sleep_enable_timer_wakeup(sleep_seconds * 1000000ULL);
    esp_deep_sleep_start();
}
