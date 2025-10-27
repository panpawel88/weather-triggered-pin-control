idf_component_register(
    SRCS "test_clock.c"
    INCLUDE_DIRS "."
    REQUIRES hardware_config rtc_time
)
