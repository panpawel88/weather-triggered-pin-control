#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rtc_helper.h"
#include "sdkconfig.h"

static const char *TAG = "CLOCK_SET";

// Helper function to get integer input from user
static int get_integer_input(const char *prompt, int min, int max) {
    char input[32];
    int value;

    while (1) {
        printf("%s (%d-%d): ", prompt, min, max);
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nError reading input. Please try again.\n");
            continue;
        }

        // Remove newline
        input[strcspn(input, "\n")] = 0;

        if (sscanf(input, "%d", &value) == 1) {
            if (value >= min && value <= max) {
                return value;
            } else {
                printf("Value out of range. Please enter a value between %d and %d.\n", min, max);
            }
        } else {
            printf("Invalid input. Please enter a number.\n");
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  RTC Clock Set Application");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // I2C pins from menuconfig
    int sda_pin = CONFIG_WEATHER_CONTROL_I2C_SDA;
    int scl_pin = CONFIG_WEATHER_CONTROL_I2C_SCL;

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

    // Read current time
    datetime_t current;
    ESP_LOGI(TAG, "Reading current RTC time...");
    if (rtc_read_time(&current) == ESP_OK) {
        ESP_LOGI(TAG, "Current RTC time: %04d-%02d-%02d %02d:%02d:%02d",
                 current.year, current.month, current.day,
                 current.hour, current.minute, current.second);
        ESP_LOGI(TAG, "");
    } else {
        ESP_LOGW(TAG, "Could not read current time from RTC");
        ESP_LOGI(TAG, "");
    }

    // Get new time from user
    printf("\n");
    printf("========================================\n");
    printf("Enter new date and time\n");
    printf("========================================\n");
    printf("\n");

    datetime_t new_time;

    new_time.year = get_integer_input("Year", 2000, 2099);
    new_time.month = get_integer_input("Month", 1, 12);
    new_time.day = get_integer_input("Day", 1, 31);
    new_time.hour = get_integer_input("Hour (24h format)", 0, 23);
    new_time.minute = get_integer_input("Minute", 0, 59);
    new_time.second = get_integer_input("Second", 0, 59);

    printf("\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "New time to be set:");
    ESP_LOGI(TAG, "  %04d-%02d-%02d %02d:%02d:%02d",
             new_time.year, new_time.month, new_time.day,
             new_time.hour, new_time.minute, new_time.second);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    printf("Confirm setting this time? (y/n): ");
    fflush(stdout);

    char confirm[16];
    if (fgets(confirm, sizeof(confirm), stdin) != NULL) {
        confirm[strcspn(confirm, "\n")] = 0;

        if (strcmp(confirm, "y") == 0 || strcmp(confirm, "Y") == 0 ||
            strcmp(confirm, "yes") == 0 || strcmp(confirm, "YES") == 0) {

            ESP_LOGI(TAG, "Writing time to RTC...");
            if (rtc_write_time(&new_time) == ESP_OK) {
                ESP_LOGI(TAG, "Time set successfully!");
                ESP_LOGI(TAG, "");

                // Verify by reading back
                ESP_LOGI(TAG, "Verifying...");
                vTaskDelay(100 / portTICK_PERIOD_MS);

                datetime_t verify;
                if (rtc_read_time(&verify) == ESP_OK) {
                    ESP_LOGI(TAG, "RTC now reads: %04d-%02d-%02d %02d:%02d:%02d",
                             verify.year, verify.month, verify.day,
                             verify.hour, verify.minute, verify.second);
                    ESP_LOGI(TAG, "");
                    ESP_LOGI(TAG, "Clock set operation complete!");
                }
            } else {
                ESP_LOGE(TAG, "Failed to write time to RTC!");
            }
        } else {
            ESP_LOGI(TAG, "Operation cancelled by user");
        }
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Application will now exit");
}
