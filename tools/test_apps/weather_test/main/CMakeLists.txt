idf_component_register(
    SRCS "test_weather.c"
    INCLUDE_DIRS "."
    REQUIRES hardware_config rtc_time weather_client wifi_helper
)
