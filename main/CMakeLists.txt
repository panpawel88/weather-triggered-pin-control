idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES hardware_config rtc_time led_gpio weather_client rgb_status_led config_utils wifi_helper remote_logging esp_wifi esp_event esp_http_client nvs_flash driver json)