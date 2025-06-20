idf_component_register(SRCS "imu.c" "screen.c" "hello_world_main.c"
                    PRIV_REQUIRES spi_flash esp_timer driver nvs_flash I2Cdev st7789 spiffs
                    INCLUDE_DIRS ""
                    REQUIRES driver)
