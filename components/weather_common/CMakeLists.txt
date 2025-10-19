idf_component_register(
    SRCS
        "rtc_helper.c"
        "led_control.c"
        "weather_fetch.c"
        "config_print.c"
    INCLUDE_DIRS
        "include"
        "${CMAKE_SOURCE_DIR}/main"  # Include main/ for config.h access
    REQUIRES
        driver
        esp_wifi
        esp_event
        esp_http_client
        nvs_flash
        json
)
