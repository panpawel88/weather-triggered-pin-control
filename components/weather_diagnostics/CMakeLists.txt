idf_component_register(SRCS "weather_diagnostics.c"
                    INCLUDE_DIRS "include"
                    REQUIRES hardware_config rtc_time weather_client esp_http_client esp_wifi nvs_flash)
