idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES weather_common remote_logging esp_wifi esp_event esp_http_client nvs_flash driver json)