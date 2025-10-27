#ifndef TIMEZONE_HELPER_H
#define TIMEZONE_HELPER_H

#include "esp_err.h"
#include "rtc_helper.h"
#include <time.h>

/**
 * @brief Initialize timezone settings from hardware_config.h
 *
 * Sets the TZ environment variable and calls tzset() to enable
 * automatic DST conversion for the configured timezone.
 *
 * @return ESP_OK on success
 */
esp_err_t timezone_init(void);

/**
 * @brief Convert UTC datetime to local time with DST adjustment
 *
 * @param utc_dt Pointer to datetime_t structure containing UTC time
 * @param local_dt Pointer to datetime_t structure to store local time
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t utc_to_local(const datetime_t *utc_dt, datetime_t *local_dt);

/**
 * @brief Convert local time to UTC datetime
 *
 * @param local_dt Pointer to datetime_t structure containing local time
 * @param utc_dt Pointer to datetime_t structure to store UTC time
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t local_to_utc(const datetime_t *local_dt, datetime_t *utc_dt);

/**
 * @brief Get timezone offset in seconds for a given UTC time
 *
 * Returns the offset including DST adjustment.
 *
 * @param utc_dt Pointer to datetime_t structure containing UTC time
 * @param offset_seconds Pointer to store offset in seconds (positive = east of UTC)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t get_timezone_offset(const datetime_t *utc_dt, int *offset_seconds);

/**
 * @brief Get timezone abbreviation for a given UTC time
 *
 * Returns "CET" or "CEST" depending on whether DST is active.
 *
 * @param utc_dt Pointer to datetime_t structure containing UTC time
 * @param tz_abbr Buffer to store timezone abbreviation (at least 8 bytes)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t get_timezone_abbr(const datetime_t *utc_dt, char *tz_abbr);

#endif // TIMEZONE_HELPER_H
