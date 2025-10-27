idf_component_register(
    SRCS "rgb_led_test.c"
    INCLUDE_DIRS "."
    REQUIRES hardware_config rgb_status_led
)
