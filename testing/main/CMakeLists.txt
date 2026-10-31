idf_component_register(SRCS "testing.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_timer esp_rom freertos esp_driver_gpio)