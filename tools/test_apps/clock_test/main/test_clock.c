#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rtc_helper.h"
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
        datetime_t now;

        if (rtc_read_time(&now) == ESP_OK) {
            // Calculate day of week (simple Zeller's algorithm approximation)
            // This is just for display, DS3231 stores it but we don't use it in our system
            int day_of_week = (now.day + (13 * (now.month + 1)) / 5 +
                              now.year + now.year / 4 - now.year / 100 +
                              now.year / 400) % 7;

            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "Read #%d", read_count);
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "Date: %s, %s %d, %d",
                     day_names[day_of_week],
                     month_names[now.month],
                     now.day,
                     now.year);
            ESP_LOGI(TAG, "Time: %02d:%02d:%02d (24-hour format)",
                     now.hour, now.minute, now.second);
            ESP_LOGI(TAG, "ISO 8601: %04d-%02d-%02d %02d:%02d:%02d",
                     now.year, now.month, now.day,
                     now.hour, now.minute, now.second);
            ESP_LOGI(TAG, "");
        } else {
            ESP_LOGE(TAG, "Failed to read time from RTC");
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
