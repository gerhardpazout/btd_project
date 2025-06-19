#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <dirent.h>
#include "imu.h"  // Sensor Library from TUWEL
#include <math.h>
#include <string.h>

#include "driver/i2c.h"
#include "st7789.h"
#include "axp192.h"
#include "esp_spiffs.h"
#include "esp_timer.h"

#define TAG "BUTTON_ISR"

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

///////////////////////////////////////////////////
// buttons
///////////////////////////////////////////////////
#define BUTTON_A_GPIO   37      // orange front
#define BUTTON_B_GPIO   39      // side (if fitted)
#define DOUBLE_WIN_MS   400     // double-click window

///////////////////////////////////////////////////
// globals
///////////////////////////////////////////////////
static TaskHandle_t  button_task_handle = NULL;
static TimerHandle_t single_tmr         = NULL;
static volatile int  button_pressed     = -1;
static volatile bool waiting_second     = false;

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

/**
 * Called by timer for single press detection
 */
static void single_press_cb(TimerHandle_t xTimer)
{
    waiting_second = false;
    ESP_LOGI(TAG, "Button A SINGLE press");
}

/**
 * ISR handler for buttons
 */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    button_pressed = (int) arg;               // which pin
    xTaskNotifyFromISR(button_task_handle, 0, eNoAction, NULL);
}

/**
 * Task for handling button interaction
 */
static void button_task(void *arg)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // debounce (to prevent accidental additional button registration during click)
        vTaskDelay(pdMS_TO_TICKS(40));
        if (button_pressed == BUTTON_A_GPIO && gpio_get_level(BUTTON_A_GPIO) == 0) {
            // Button A logic
            if (waiting_second) {
                // second press inside window -> DOUBLE
                xTimerStop(single_tmr, 0);
                waiting_second = false;
                ESP_LOGI(TAG, "Button A DOUBLE press");
            } else {
                // first press: prepare one-shot timer
                waiting_second = true;
                xTimerStart(single_tmr, 0);          // 400 ms window
            }
        }
        else if (button_pressed == BUTTON_B_GPIO && gpio_get_level(BUTTON_B_GPIO) == 0) {
            // Button B logic
            ESP_LOGI(TAG, "Button B press");
        }

        button_pressed = -1;     // ready for next interrupt
    }
}

/**
 * Main
 */
void app_main(void)
{
    init_spiffs();

    DIR* dir = opendir("/spiffs");
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            printf("SPIFFS file: %s\n", ent->d_name);
        }
        closedir(dir);
    }

     // Init I2C & MPU
    ESP_LOGI(TAG, "Initializing I2C and MPU...");
    ESP_ERROR_CHECK(nvs_flash_init());
    i2c_master_init_TUWEL();
    init_mpu6886();
    ESP_LOGI(TAG, "I2C and MPU initialized!");

    // Power on
    AXP192_PowerOn();
    AXP192_ScreenBreath(11);

    // Init display
    TFT_t dev;
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
    lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY, CONFIG_INVERSION);

    // Setup font
    FontxFile fx[2];
    InitFontx(fx, "/spiffs/ILGH16XB.FNT", NULL);
    lcdSetFontDirection(&dev, 1);

    lcdFillScreen(&dev, BLUE); // initial background color of the display

    lcdDrawString(&dev, fx, 10, 10, (uint8_t *)"Hello World", WHITE);


    // GPIO setup
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_A_GPIO) | (1ULL << BUTTON_B_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);

    // create one-shot timer for single-press detection
    single_tmr = xTimerCreate("single",
                              pdMS_TO_TICKS(DOUBLE_WIN_MS),
                              pdFALSE,         // one-shot
                              NULL,
                              single_press_cb);

    // start task
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, &button_task_handle);

    // install ISR (after task exists so handle is valid)
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_A_GPIO, button_isr_handler, (void*) BUTTON_A_GPIO);
    gpio_isr_handler_add(BUTTON_B_GPIO, button_isr_handler, (void*) BUTTON_B_GPIO);

    ESP_LOGI(TAG, "Ready! Button A: single/double | Button B: single");
}
