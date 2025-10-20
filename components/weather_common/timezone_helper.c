#include "timezone_helper.h"
#include "hardware_config.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>
#include <string.h>

static const char *TAG = "TIMEZONE";

esp_err_t timezone_init(void) {
    // Set timezone from hardware config
    setenv("TZ", HW_TIMEZONE_POSIX, 1);
    tzset();

    ESP_LOGI(TAG, "Timezone initialized: %s", HW_TIMEZONE_POSIX);
    return ESP_OK;
}

// ESP-IDF compatible timegm replacement
// Converts UTC time to time_t
static time_t portable_timegm(struct tm *tm) {
    time_t ret;
    char *tz;

    // Save current timezone
    tz = getenv("TZ");

    // Temporarily set to UTC
    setenv("TZ", "UTC0", 1);
    tzset();

    // Use mktime (which now treats input as UTC)
    ret = mktime(tm);

    // Restore original timezone
    if (tz) {
        setenv("TZ", tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();

    return ret;
}

esp_err_t utc_to_local(const datetime_t *utc_dt, datetime_t *local_dt) {
    if (!utc_dt || !local_dt) {
        return ESP_ERR_INVALID_ARG;
    }

    // Convert UTC datetime to time_t (seconds since epoch)
    struct tm utc_tm = {0};
    utc_tm.tm_year = utc_dt->year - 1900;
    utc_tm.tm_mon = utc_dt->month - 1;
    utc_tm.tm_mday = utc_dt->day;
    utc_tm.tm_hour = utc_dt->hour;
    utc_tm.tm_min = utc_dt->minute;
    utc_tm.tm_sec = utc_dt->second;
    utc_tm.tm_isdst = 0;  // UTC has no DST

    // portable_timegm() converts struct tm to time_t treating it as UTC
    time_t utc_time = portable_timegm(&utc_tm);
    if (utc_time == (time_t)-1) {
        ESP_LOGE(TAG, "Failed to convert UTC time");
        return ESP_FAIL;
    }

    // Convert time_t to local time using configured timezone
    struct tm local_tm;
    localtime_r(&utc_time, &local_tm);

    local_dt->year = local_tm.tm_year + 1900;
    local_dt->month = local_tm.tm_mon + 1;
    local_dt->day = local_tm.tm_mday;
    local_dt->hour = local_tm.tm_hour;
    local_dt->minute = local_tm.tm_min;
    local_dt->second = local_tm.tm_sec;

    ESP_LOGD(TAG, "UTC->Local: %04d-%02d-%02d %02d:%02d:%02d -> %04d-%02d-%02d %02d:%02d:%02d",
             utc_dt->year, utc_dt->month, utc_dt->day, utc_dt->hour, utc_dt->minute, utc_dt->second,
             local_dt->year, local_dt->month, local_dt->day, local_dt->hour, local_dt->minute, local_dt->second);

    return ESP_OK;
}

esp_err_t local_to_utc(const datetime_t *local_dt, datetime_t *utc_dt) {
    if (!local_dt || !utc_dt) {
        return ESP_ERR_INVALID_ARG;
    }

    // Convert local datetime to time_t
    struct tm local_tm = {0};
    local_tm.tm_year = local_dt->year - 1900;
    local_tm.tm_mon = local_dt->month - 1;
    local_tm.tm_mday = local_dt->day;
    local_tm.tm_hour = local_dt->hour;
    local_tm.tm_min = local_dt->minute;
    local_tm.tm_sec = local_dt->second;
    local_tm.tm_isdst = -1;  // Let mktime determine DST

    // mktime() converts struct tm to time_t treating it as local time
    time_t local_time = mktime(&local_tm);
    if (local_time == (time_t)-1) {
        ESP_LOGE(TAG, "Failed to convert local time");
        return ESP_FAIL;
    }

    // Convert time_t to UTC
    struct tm utc_tm;
    gmtime_r(&local_time, &utc_tm);

    utc_dt->year = utc_tm.tm_year + 1900;
    utc_dt->month = utc_tm.tm_mon + 1;
    utc_dt->day = utc_tm.tm_mday;
    utc_dt->hour = utc_tm.tm_hour;
    utc_dt->minute = utc_tm.tm_min;
    utc_dt->second = utc_tm.tm_sec;

    ESP_LOGD(TAG, "Local->UTC: %04d-%02d-%02d %02d:%02d:%02d -> %04d-%02d-%02d %02d:%02d:%02d",
             local_dt->year, local_dt->month, local_dt->day, local_dt->hour, local_dt->minute, local_dt->second,
             utc_dt->year, utc_dt->month, utc_dt->day, utc_dt->hour, utc_dt->minute, utc_dt->second);

    return ESP_OK;
}

esp_err_t get_timezone_offset(const datetime_t *utc_dt, int *offset_seconds) {
    if (!utc_dt || !offset_seconds) {
        return ESP_ERR_INVALID_ARG;
    }

    // Convert UTC to time_t
    struct tm utc_tm = {0};
    utc_tm.tm_year = utc_dt->year - 1900;
    utc_tm.tm_mon = utc_dt->month - 1;
    utc_tm.tm_mday = utc_dt->day;
    utc_tm.tm_hour = utc_dt->hour;
    utc_tm.tm_min = utc_dt->minute;
    utc_tm.tm_sec = utc_dt->second;
    utc_tm.tm_isdst = 0;

    time_t utc_time = portable_timegm(&utc_tm);
    if (utc_time == (time_t)-1) {
        return ESP_FAIL;
    }

    // Get local time
    struct tm local_tm;
    localtime_r(&utc_time, &local_tm);

    // Convert both to time_t and calculate difference
    // Since tm_gmtoff is not available in ESP-IDF, calculate manually
    struct tm utc_tm_check;
    gmtime_r(&utc_time, &utc_tm_check);

    time_t local_as_utc = portable_timegm(&local_tm);
    time_t utc_as_utc = utc_time;

    // The offset is the difference
    *offset_seconds = (int)difftime(local_as_utc, utc_as_utc);

    return ESP_OK;
}

esp_err_t get_timezone_abbr(const datetime_t *utc_dt, char *tz_abbr) {
    if (!utc_dt || !tz_abbr) {
        return ESP_ERR_INVALID_ARG;
    }

    // Convert UTC to time_t
    struct tm utc_tm = {0};
    utc_tm.tm_year = utc_dt->year - 1900;
    utc_tm.tm_mon = utc_dt->month - 1;
    utc_tm.tm_mday = utc_dt->day;
    utc_tm.tm_hour = utc_dt->hour;
    utc_tm.tm_min = utc_dt->minute;
    utc_tm.tm_sec = utc_dt->second;
    utc_tm.tm_isdst = 0;

    time_t utc_time = portable_timegm(&utc_tm);
    if (utc_time == (time_t)-1) {
        return ESP_FAIL;
    }

    // Get local time
    struct tm local_tm;
    localtime_r(&utc_time, &local_tm);

    // Get timezone abbreviation using strftime with %Z format
    // Since tm_zone is not available in ESP-IDF, use strftime
    if (strftime(tz_abbr, 8, "%Z", &local_tm) == 0) {
        // Fallback if strftime fails
        strcpy(tz_abbr, "???");
    }

    return ESP_OK;
}
