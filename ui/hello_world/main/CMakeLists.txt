idf_component_register(SRCS "imu.c" "screen.c" "datacapture.c" "datatransfer.c" "wifi.c" "hello_world_main.c"
                    PRIV_REQUIRES spi_flash esp_timer esp_wifi esp_event driver nvs_flash I2Cdev st7789 spiffs
                    INCLUDE_DIRS ""
                    REQUIRES driver)
