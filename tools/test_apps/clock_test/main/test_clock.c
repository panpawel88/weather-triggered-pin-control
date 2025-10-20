#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rtc_helper.h"
#include "timezone_helper.h"
#include "hardware_config.h"

static const char *TAG = "CLOCK_TEST";

// Day of week names
static const char *day_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

// Month names
static const char *month_names[] = {
    "", "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  RTC Clock Test Application");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Initialize timezone for DST support
    ESP_LOGI(TAG, "Initializing timezone: %s", HW_TIMEZONE_POSIX);
    if (timezone_init() != ESP_OK) {
        ESP_LOGE(TAG, "Timezone initialization failed!");
        return;
    }
    ESP_LOGI(TAG, "Timezone initialized successfully");
    ESP_LOGI(TAG, "");

    // I2C pins from hardware_config.h (same as main application)
    int sda_pin = HW_I2C_SDA_PIN;
    int scl_pin = HW_I2C_SCL_PIN;

    ESP_LOGI(TAG, "I2C Configuration:");
    ESP_LOGI(TAG, "  SDA Pin: %d", sda_pin);
    ESP_LOGI(TAG, "  SCL Pin: %d", scl_pin);
    ESP_LOGI(TAG, "");

    // Initialize I2C
    ESP_LOGI(TAG, "Initializing I2C...");
    if (rtc_i2c_init(sda_pin, scl_pin) != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed!");
        return;
    }

    ESP_LOGI(TAG, "I2C initialized successfully");
    ESP_LOGI(TAG, "");

    ESP_LOGI(TAG, "Reading time from DS3231 RTC every 2 seconds...");
    ESP_LOGI(TAG, "Press Ctrl+C to stop");
    ESP_LOGI(TAG, "");

    int read_count = 0;
    while (1) {
        read_count++;
        datetime_t utc_time;

        if (rtc_read_time(&utc_time) == ESP_OK) {
            // Convert UTC to local time
            datetime_t local_time;
            char tz_abbr[8] = "???";
            int tz_offset_seconds = 0;

            bool conversion_ok = (utc_to_local(&utc_time, &local_time) == ESP_OK);
            if (conversion_ok) {
                get_timezone_abbr(&utc_time, tz_abbr);
                get_timezone_offset(&utc_time, &tz_offset_seconds);
            }

            // Calculate day of week for local time (simple Zeller's algorithm approximation)
            int day_of_week = (local_time.day + (13 * (local_time.month + 1)) / 5 +
                              local_time.year + local_time.year / 4 - local_time.year / 100 +
                              local_time.year / 400) % 7;

            int tz_offset_hours = tz_offset_seconds / 3600;
            int tz_offset_mins = (abs(tz_offset_seconds) % 3600) / 60;

            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "Read #%d", read_count);
            ESP_LOGI(TAG, "========================================");

            if (conversion_ok) {
                ESP_LOGI(TAG, "UTC Time:   %04d-%02d-%02d %02d:%02d:%02d",
                         utc_time.year, utc_time.month, utc_time.day,
                         utc_time.hour, utc_time.minute, utc_time.second);
                ESP_LOGI(TAG, "Local Time: %04d-%02d-%02d %02d:%02d:%02d %s (UTC%+d:%02d)",
                         local_time.year, local_time.month, local_time.day,
                         local_time.hour, local_time.minute, local_time.second,
                         tz_abbr, tz_offset_hours, tz_offset_mins);
                ESP_LOGI(TAG, "");
                ESP_LOGI(TAG, "Date: %s, %s %d, %d",
                         day_names[day_of_week],
                         month_names[local_time.month],
                         local_time.day,
                         local_time.year);
                ESP_LOGI(TAG, "Time: %02d:%02d:%02d (24-hour format, %s)",
                         local_time.hour, local_time.minute, local_time.second, tz_abbr);
            } else {
                ESP_LOGI(TAG, "UTC Time: %04d-%02d-%02d %02d:%02d:%02d",
                         utc_time.year, utc_time.month, utc_time.day,
                         utc_time.hour, utc_time.minute, utc_time.second);
                ESP_LOGW(TAG, "Timezone conversion failed");
            }
            ESP_LOGI(TAG, "");
        } else {
            ESP_LOGE(TAG, "Failed to read time from RTC");
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
