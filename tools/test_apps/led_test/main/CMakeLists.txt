idf_component_register(
    SRCS "test_led.c"
    INCLUDE_DIRS "."
    REQUIRES hardware_config led_gpio
)
