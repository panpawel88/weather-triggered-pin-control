#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "rtc_helper.h"
#include "timezone_helper.h"
#include "hardware_config.h"

static const char *TAG = "CLOCK_SET";

// Scan I2C bus to check for devices
static void scan_i2c_bus(void) {
    ESP_LOGI(TAG, "Scanning I2C bus...");
    int found = 0;

    // Scan valid I2C addresses (0x08-0x77, skipping reserved addresses)
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        // Create a simple command link to probe the device
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 100 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  Found device at address: 0x%02X", addr);
            found++;
        }
    }

    if (found == 0) {
        ESP_LOGW(TAG, "No I2C devices found!");
        ESP_LOGW(TAG, "");
        ESP_LOGW(TAG, "Troubleshooting steps:");
        ESP_LOGW(TAG, "  1. Check DS3231 is powered (VCC and GND connected)");
        ESP_LOGW(TAG, "  2. Verify I2C pins - SDA: GPIO%d, SCL: GPIO%d", HW_I2C_SDA_PIN, HW_I2C_SCL_PIN);
        ESP_LOGW(TAG, "  3. Check pull-up resistors on SDA and SCL (4.7k-10k ohm)");
        ESP_LOGW(TAG, "  4. Verify DS3231 module is functional");
        ESP_LOGW(TAG, "  5. Try running 'clock_test' to see if reading works");
    } else {
        ESP_LOGI(TAG, "Total devices found: %d", found);
        if (found > 0) {
            ESP_LOGI(TAG, "Expected DS3231 at address: 0x68");
        }
    }
    ESP_LOGI(TAG, "");
}

// Configure stdin for interactive input
static esp_err_t configure_stdin(void) {
    // Disable buffering on stdin - this is essential for interactive input
    setvbuf(stdin, NULL, _IONBF, 0);

    // Set line endings for UART
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    // Configure UART parameters
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // First, check if UART driver is already installed
    // If not installed, install it for interrupt-driven I/O
    esp_err_t ret = uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0);
    if (ret == ESP_ERR_INVALID_STATE) {
        // Driver already installed - that's fine for console UART
        ESP_LOGD(TAG, "UART driver already installed");
    } else if (ret == ESP_OK) {
        ESP_LOGD(TAG, "UART driver installed successfully");

        // Configure UART parameters
        ret = uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(ret));
            return ret;
        }

        // Tell VFS to use UART driver
        esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    } else {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

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

    // Scan I2C bus for devices (diagnostic)
    scan_i2c_bus();

    // Initialize DS3231 device (enable oscillator, clear flags)
    ESP_LOGI(TAG, "Initializing DS3231 RTC device...");
    esp_err_t init_err = rtc_init_device();
    if (init_err != ESP_OK) {
        ESP_LOGW(TAG, "DS3231 device initialization had issues (continuing anyway)");
        ESP_LOGW(TAG, "This might affect write operations. Error: %s", esp_err_to_name(init_err));
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Attempting to proceed with current configuration...");
    } else {
        ESP_LOGI(TAG, "DS3231 device initialized successfully");
    }
    ESP_LOGI(TAG, "");

    // Read current time (stored as UTC)
    datetime_t current_utc;
    ESP_LOGI(TAG, "Reading current RTC time...");
    if (rtc_read_time(&current_utc) == ESP_OK) {
        // Convert to local time for display
        datetime_t current_local;
        char tz_abbr[8];
        if (utc_to_local(&current_utc, &current_local) == ESP_OK &&
            get_timezone_abbr(&current_utc, tz_abbr) == ESP_OK) {
            ESP_LOGI(TAG, "Current UTC time:   %04d-%02d-%02d %02d:%02d:%02d",
                     current_utc.year, current_utc.month, current_utc.day,
                     current_utc.hour, current_utc.minute, current_utc.second);
            ESP_LOGI(TAG, "Current local time: %04d-%02d-%02d %02d:%02d:%02d %s",
                     current_local.year, current_local.month, current_local.day,
                     current_local.hour, current_local.minute, current_local.second, tz_abbr);
        } else {
            ESP_LOGI(TAG, "Current RTC time (UTC): %04d-%02d-%02d %02d:%02d:%02d",
                     current_utc.year, current_utc.month, current_utc.day,
                     current_utc.hour, current_utc.minute, current_utc.second);
        }
        ESP_LOGI(TAG, "");
    } else {
        ESP_LOGW(TAG, "Could not read current time from RTC");
        ESP_LOGI(TAG, "");
    }

    // Configure stdin for interactive input (do this AFTER I2C initialization)
    ESP_LOGI(TAG, "Configuring UART for interactive input...");
    if (configure_stdin() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure stdin!");
        ESP_LOGE(TAG, "Cannot continue without interactive input capability.");
        return;
    }
    ESP_LOGI(TAG, "UART configured successfully");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Ready for keyboard input.");
    ESP_LOGI(TAG, "NOTE: Use PuTTY (Windows) or screen/minicom (Linux/Mac)");
    ESP_LOGI(TAG, "      for best results. idf.py monitor may not work properly.");
    ESP_LOGI(TAG, "");

    // Small delay to ensure UART is ready
    vTaskDelay(200 / portTICK_PERIOD_MS);

    // Get new time from user (in local time)
    printf("\n");
    printf("========================================\n");
    printf("Enter new date and time (LOCAL TIME)\n");
    printf("Timezone: %s\n", HW_TIMEZONE_POSIX);
    printf("========================================\n");
    printf("\n");

    datetime_t new_local_time;

    new_local_time.year = get_integer_input("Year", 2000, 2099);
    new_local_time.month = get_integer_input("Month", 1, 12);
    new_local_time.day = get_integer_input("Day", 1, 31);
    new_local_time.hour = get_integer_input("Hour (24h format)", 0, 23);
    new_local_time.minute = get_integer_input("Minute", 0, 59);
    new_local_time.second = get_integer_input("Second", 0, 59);

    // Convert local time to UTC for storage
    datetime_t new_utc_time;
    if (local_to_utc(&new_local_time, &new_utc_time) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to convert local time to UTC!");
        return;
    }

    // Get timezone abbreviation
    char tz_abbr[8];
    get_timezone_abbr(&new_utc_time, tz_abbr);

    printf("\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Time conversion:");
    ESP_LOGI(TAG, "  Local time: %04d-%02d-%02d %02d:%02d:%02d %s",
             new_local_time.year, new_local_time.month, new_local_time.day,
             new_local_time.hour, new_local_time.minute, new_local_time.second, tz_abbr);
    ESP_LOGI(TAG, "  UTC time:   %04d-%02d-%02d %02d:%02d:%02d",
             new_utc_time.year, new_utc_time.month, new_utc_time.day,
             new_utc_time.hour, new_utc_time.minute, new_utc_time.second);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    printf("Confirm setting this time? (y/n): ");
    fflush(stdout);

    char confirm[16];
    if (fgets(confirm, sizeof(confirm), stdin) != NULL) {
        confirm[strcspn(confirm, "\n")] = 0;

        if (strcmp(confirm, "y") == 0 || strcmp(confirm, "Y") == 0 ||
            strcmp(confirm, "yes") == 0 || strcmp(confirm, "YES") == 0) {

            ESP_LOGI(TAG, "Writing UTC time to RTC...");
            if (rtc_write_time(&new_utc_time) == ESP_OK) {
                ESP_LOGI(TAG, "Time set successfully!");
                ESP_LOGI(TAG, "");

                // Verify by reading back
                ESP_LOGI(TAG, "Verifying...");
                vTaskDelay(100 / portTICK_PERIOD_MS);

                datetime_t verify_utc;
                if (rtc_read_time(&verify_utc) == ESP_OK) {
                    datetime_t verify_local;
                    char verify_tz[8];
                    if (utc_to_local(&verify_utc, &verify_local) == ESP_OK &&
                        get_timezone_abbr(&verify_utc, verify_tz) == ESP_OK) {
                        ESP_LOGI(TAG, "RTC now reads (UTC):   %04d-%02d-%02d %02d:%02d:%02d",
                                 verify_utc.year, verify_utc.month, verify_utc.day,
                                 verify_utc.hour, verify_utc.minute, verify_utc.second);
                        ESP_LOGI(TAG, "RTC now reads (Local): %04d-%02d-%02d %02d:%02d:%02d %s",
                                 verify_local.year, verify_local.month, verify_local.day,
                                 verify_local.hour, verify_local.minute, verify_local.second, verify_tz);
                    }
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
