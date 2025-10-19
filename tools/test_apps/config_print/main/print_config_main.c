#include <stdio.h>
#include "esp_log.h"
#include "config_print.h"

static const char *TAG = "CONFIG_PRINT";

void app_main(void) {
    ESP_LOGI(TAG, "Configuration Print Application");
    ESP_LOGI(TAG, "");

    // Print all configuration using the shared component
    print_all_config();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Configuration print complete");
}
