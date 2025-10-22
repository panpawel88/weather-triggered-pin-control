idf_component_register(SRCS "remote_logging.c"
                    INCLUDE_DIRS "include"
                    REQUIRES weather_common esp_http_client esp_wifi nvs_flash)
