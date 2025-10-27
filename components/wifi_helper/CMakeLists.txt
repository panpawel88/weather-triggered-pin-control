idf_component_register(SRCS "wifi_helper.c"
                    INCLUDE_DIRS "include"
                    REQUIRES hardware_config esp_wifi esp_event esp_netif nvs_flash)
