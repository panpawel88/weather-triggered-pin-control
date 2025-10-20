#include "rtc_helper.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "RTC_HELPER";

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

// Convert BCD to decimal
uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Convert decimal to BCD
uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

esp_err_t rtc_i2c_init(int sda_pin, int scl_pin) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C initialized (SDA=%d, SCL=%d)", sda_pin, scl_pin);
    return ESP_OK;
}

esp_err_t rtc_init_device(void) {
    esp_err_t ret;
    uint8_t control_reg, status_reg;

    // Read current control register
    ret = i2c_master_write_read_device(
        I2C_MASTER_NUM,
        DS3231_ADDR,
        (uint8_t[]){DS3231_REG_CONTROL}, 1,
        &control_reg, 1,
        1000 / portTICK_PERIOD_MS
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read control register: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "DS3231 Control register: 0x%02X", control_reg);

    // Read current status register
    ret = i2c_master_write_read_device(
        I2C_MASTER_NUM,
        DS3231_ADDR,
        (uint8_t[]){DS3231_REG_STATUS}, 1,
        &status_reg, 1,
        1000 / portTICK_PERIOD_MS
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read status register: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "DS3231 Status register: 0x%02X", status_reg);

    // Check if oscillator was stopped
    if (status_reg & DS3231_STATUS_OSF) {
        ESP_LOGW(TAG, "Oscillator Stop Flag (OSF) is set - RTC may have invalid time");
    }

    // Ensure oscillator is enabled (EOSC bit = 0)
    // Set INTCN = 1 to enable interrupt output instead of square wave
    uint8_t new_control = control_reg & ~DS3231_CONTROL_EOSC;  // Clear EOSC (enable oscillator)
    new_control |= DS3231_CONTROL_INTCN;  // Set INTCN

    if (new_control != control_reg) {
        ESP_LOGI(TAG, "Updating control register: 0x%02X -> 0x%02X", control_reg, new_control);
        ret = i2c_master_write_to_device(
            I2C_MASTER_NUM,
            DS3231_ADDR,
            (uint8_t[]){DS3231_REG_CONTROL, new_control}, 2,
            1000 / portTICK_PERIOD_MS
        );

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write control register: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    // Clear OSF flag in status register
    if (status_reg & DS3231_STATUS_OSF) {
        uint8_t new_status = status_reg & ~DS3231_STATUS_OSF;  // Clear OSF bit
        ESP_LOGI(TAG, "Clearing Oscillator Stop Flag: 0x%02X -> 0x%02X", status_reg, new_status);

        ret = i2c_master_write_to_device(
            I2C_MASTER_NUM,
            DS3231_ADDR,
            (uint8_t[]){DS3231_REG_STATUS, new_status}, 2,
            1000 / portTICK_PERIOD_MS
        );

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to clear OSF flag: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    ESP_LOGI(TAG, "DS3231 device initialized successfully");
    return ESP_OK;
}

esp_err_t rtc_read_time(datetime_t *dt) {
    if (!dt) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[7];
    esp_err_t ret = i2c_master_write_read_device(
        I2C_MASTER_NUM,
        DS3231_ADDR,
        (uint8_t[]){0x00}, 1,
        data, 7,
        1000 / portTICK_PERIOD_MS
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read from DS3231: %s", esp_err_to_name(ret));
        return ret;
    }

    dt->second = bcd_to_dec(data[0] & 0x7F);
    dt->minute = bcd_to_dec(data[1] & 0x7F);
    dt->hour = bcd_to_dec(data[2] & 0x3F);
    dt->day = bcd_to_dec(data[4] & 0x3F);
    dt->month = bcd_to_dec(data[5] & 0x1F);
    dt->year = bcd_to_dec(data[6]) + 2000;

    ESP_LOGD(TAG, "Read time (UTC): %04d-%02d-%02d %02d:%02d:%02d",
             dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->second);

    return ESP_OK;
}

esp_err_t rtc_write_time(const datetime_t *dt) {
    if (!dt) {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate input ranges
    if (dt->year < 2000 || dt->year > 2099 ||
        dt->month < 1 || dt->month > 12 ||
        dt->day < 1 || dt->day > 31 ||
        dt->hour > 23 || dt->minute > 59 || dt->second > 59) {
        ESP_LOGE(TAG, "Invalid datetime values");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[8];
    data[0] = 0x00;  // Register address
    data[1] = dec_to_bcd(dt->second);
    data[2] = dec_to_bcd(dt->minute);
    data[3] = dec_to_bcd(dt->hour);
    data[4] = 1;  // Day of week (not used, set to Monday)
    data[5] = dec_to_bcd(dt->day);
    data[6] = dec_to_bcd(dt->month);
    data[7] = dec_to_bcd(dt->year - 2000);

    ESP_LOGD(TAG, "Writing time data: %02X %02X %02X %02X %02X %02X %02X %02X",
             data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

    esp_err_t ret = i2c_master_write_to_device(
        I2C_MASTER_NUM,
        DS3231_ADDR,
        data, 8,
        1000 / portTICK_PERIOD_MS
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to DS3231: %s (0x%X)", esp_err_to_name(ret), ret);
        ESP_LOGE(TAG, "This usually indicates an I2C communication issue or device not responding");
        return ret;
    }

    ESP_LOGI(TAG, "Time set to (UTC): %04d-%02d-%02d %02d:%02d:%02d",
             dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->second);

    return ESP_OK;
}
