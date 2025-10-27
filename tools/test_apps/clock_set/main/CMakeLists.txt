idf_component_register(
    SRCS "set_clock.c"
    INCLUDE_DIRS "."
    REQUIRES hardware_config rtc_time driver vfs
)
