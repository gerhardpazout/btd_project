#include "screen.h"
#include <stdio.h>
#include <dirent.h>
#include "esp_log.h"
#include "time_simple.h"
#include "shared_globals.h"

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

#define HIGHLIGHT_HEIGHT 20
#define HIGHLIGHT_WIDTH 15

#define TAG "SCREEN"

TFT_t dev;
FontxFile fx[2];
int test = 0;

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
    // TFT_t dev;
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
    lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY, CONFIG_INVERSION);

    // Setup font
    // FontxFile fx[2];
    InitFontx(fx, "/spiffs/ILGH16XB.FNT", NULL);
    lcdSetFontDirection(&dev, 1);

    lcdFillScreen(&dev, BLUE); // initial background color of the display

    lcdDrawString(&dev, fx, 10, 10, (uint8_t *)"Hello World (Screen 5)", WHITE);
}

void draw_highlight(uint8_t index) {
    char buf[32];
    int rect_x = 0;
    int rect_y = 0;
    int text_x = 0;
    int text_y = 0;
    
    switch (index) {
        case 1:
            snprintf(buf, sizeof(buf), "%02d", alarm_start.minute);
            rect_x = 50;
            rect_y = 40;
            text_x = 50;
            text_y = 40;
            break;
        case 2:
            snprintf(buf, sizeof(buf), "%02d", alarm_end.hour);
            rect_x = 50;
            rect_y = 80;
            text_x = 50;
            text_y = 80;
            break;
        case 3:
            snprintf(buf, sizeof(buf), "%02d", alarm_end.minute);
            rect_x = 50;
            rect_y = 110;
            text_x = 50;
            text_y = 110;
            break;
        default:
            snprintf(buf, sizeof(buf), "%02d", alarm_start.hour);
            rect_x = 50;
            rect_y = 10;
            text_x = 50;
            text_y = 10;
            break;
    }

    lcdDrawFillRect(&dev, rect_x, rect_y, rect_x + HIGHLIGHT_HEIGHT, rect_y + HIGHLIGHT_WIDTH, WHITE);
    lcdDrawString(&dev, fx, text_x, text_y, (uint8_t *)buf, BLUE);
}

void update_screen() {
    lcdFillScreen(&dev, BLUE);

    lcdDrawString(&dev, fx, 80, 10, (uint8_t *)"Wake me up between:", WHITE);

    char buf[32];
    //snprintf(buf, sizeof(buf), "%02d:%02d - %02d:%02d", alarm_start.hour, alarm_start.minute, alarm_end.hour, alarm_end.minute);
    // lcdDrawString(&dev, fx, 50, 10, (uint8_t *)buf, WHITE);

    // XX:00 - 00:00
    snprintf(buf, sizeof(buf), "%02d", alarm_start.hour);
    lcdDrawString(&dev, fx, 50, 10, (uint8_t *)buf, WHITE);

    // :
    snprintf(buf, sizeof(buf), ":");
    lcdDrawString(&dev, fx, 50, 30, (uint8_t *)buf, WHITE);

    // 00:XX - 00:00
    snprintf(buf, sizeof(buf), "%02d", alarm_start.minute);
    lcdDrawString(&dev, fx, 50, 40, (uint8_t *)buf, WHITE);

    // -
    snprintf(buf, sizeof(buf), "-");
    lcdDrawString(&dev, fx, 50, 65, (uint8_t *)buf, WHITE);

    // 00:00 - XX:00
    snprintf(buf, sizeof(buf), "%02d", alarm_end.hour);
    lcdDrawString(&dev, fx, 50, 80, (uint8_t *)buf, WHITE);

    // :
    snprintf(buf, sizeof(buf), ":");
    lcdDrawString(&dev, fx, 50, 100, (uint8_t *)buf, WHITE);

    // 00:00 - 00:XX
    snprintf(buf, sizeof(buf), "%02d", alarm_end.minute);
    lcdDrawString(&dev, fx, 50, 110, (uint8_t *)buf, WHITE);

    // draw highlight
    draw_highlight(alarm_index);

    


    /*
    char buf[32];
    snprintf(buf, sizeof(buf), "Test: %d s", test);
    lcdDrawString(&dev, fx, 30, 10, (uint8_t *)buf, WHITE);
    */
}

void set_test(int test_new) {
    test = test_new;
}

int get_test() {
    return test;
}

void update_alarm_on_screen(TimeSimple alarm_start_new, TimeSimple alarm_end_new) {
    alarm_start = alarm_start_new;
    alarm_end = alarm_end_new;
}