#include "screen.h"
#include <stdio.h>
#include <dirent.h>
#include "esp_log.h"

#include "sdkconfig.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "st7789.h"
#include "axp192.h"
#include "esp_spiffs.h"


// define values here so you dont need to fiddle around with idf.py menuconfig
#define CONFIG_WIDTH 135
#define CONFIG_HEIGHT 240
#define CONFIG_OFFSETX 52
#define CONFIG_OFFSETY 40
#define CONFIG_INVERSION 0
#define CONFIG_MOSI_GPIO 15
#define CONFIG_SCLK_GPIO 13
#define CONFIG_CS_GPIO 5
#define CONFIG_DC_GPIO 23
#define CONFIG_RESET_GPIO 18
#define CONFIG_BL_GPIO 27

#define TAG "SCREEN"

void list_spiffs_files() {
    DIR* dir = opendir("/spiffs");
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            printf("SPIFFS file: %s\n", ent->d_name);
        }
        closedir(dir);
    } else {
        printf("Failed to open /spiffs\n");
    }
}

void init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage1",
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    list_spiffs_files();  
    if (ret != ESP_OK) {
        ESP_LOGE("SPIFFS", "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI("SPIFFS", "SPIFFS mounted successfully");
    }
}

void init_screen() {
    init_spiffs();

    DIR* dir = opendir("/spiffs");
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            printf("SPIFFS file: %s\n", ent->d_name);
        }
        closedir(dir);
    }

    // Power on
    AXP192_PowerOn();
    AXP192_ScreenBreath(11);
}

void render_screen() {
    ESP_LOGI(TAG, "Calling init_display");

     // Init display
    TFT_t dev;
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
    lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY, CONFIG_INVERSION);

    // Setup font
    FontxFile fx[2];
    InitFontx(fx, "/spiffs/ILGH16XB.FNT", NULL);
    lcdSetFontDirection(&dev, 1);

    lcdFillScreen(&dev, BLUE); // initial background color of the display

    lcdDrawString(&dev, fx, 10, 10, (uint8_t *)"Hello World (Screen 3)", WHITE);
}