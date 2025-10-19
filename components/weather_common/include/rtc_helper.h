#ifndef RTC_HELPER_H
#define RTC_HELPER_H

#include "esp_err.h"
#include <stdint.h>

// I2C configuration for DS3231 RTC
#define DS3231_ADDR 0x68

// Date/time structure
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} datetime_t;

/**
 * @brief Initialize I2C master for DS3231 RTC
 *
 * @param sda_pin GPIO pin for I2C SDA
 * @param scl_pin GPIO pin for I2C SCL
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rtc_i2c_init(int sda_pin, int scl_pin);

/**
 * @brief Read current time from DS3231 RTC
 *
 * @param dt Pointer to datetime_t structure to store the time
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rtc_read_time(datetime_t *dt);

/**
 * @brief Write time to DS3231 RTC
 *
 * @param dt Pointer to datetime_t structure containing the time to set
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rtc_write_time(const datetime_t *dt);

/**
 * @brief Convert BCD (Binary Coded Decimal) to decimal
 *
 * @param bcd BCD value
 * @return Decimal value
 */
uint8_t bcd_to_dec(uint8_t bcd);

/**
 * @brief Convert decimal to BCD (Binary Coded Decimal)
 *
 * @param dec Decimal value
 * @return BCD value
 */
uint8_t dec_to_bcd(uint8_t dec);

#endif // RTC_HELPER_H
