#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "time_simple.h"
#include "shared_globals.h"

#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <dirent.h>
#include "imu.h"  // Sensor Library from TUWEL
#include <math.h>
#include <string.h>

#include "screen.h"

#include "driver/i2c.h"
#include "st7789.h"
#include "axp192.h"
#include "esp_spiffs.h"
#include "esp_timer.h"

#define TAG "BUTTON_ISR"



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

uint8_t alarm_index = 0;
TimeSimple alarm_start   = { .hour = 0, .minute = 0 };
TimeSimple alarm_end     = { .hour = 0, .minute = 0 };

void increase_alarm(uint8_t index) {
    switch (index) {
        case 1:
            alarm_start.minute = (alarm_start.minute + 5) % 60;
            break;
        case 2:
            alarm_end.hour = (alarm_end.hour + 1) % 24;
            break;
        case 3:
            alarm_end.minute = (alarm_end.minute + 5) % 60;
            break;
        default:
            alarm_start.hour = (alarm_start.hour + 1) % 24;
            break;
    }

    update_alarm_on_screen(alarm_start, alarm_end);
    update_screen();
}

void increase_alarm_index() {
    alarm_index = (alarm_index + 1) % 4;

    if(alarm_index == 2) {
        alarm_end.hour = alarm_start.hour;
        update_alarm_on_screen(alarm_start, alarm_end);
    }

    ESP_LOGI(TAG, "Alarm index increased (%d)", alarm_index);
    update_screen();
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

                increase_alarm_index();
            }
        }
        else if (button_pressed == BUTTON_B_GPIO && gpio_get_level(BUTTON_B_GPIO) == 0) {
            // Button B logic
            ESP_LOGI(TAG, "Button B press");
            set_test(get_test() + 1);

            increase_alarm(alarm_index);

            // log alarm timeframe
            ESP_LOGI(TAG, "%02d:%02d - %02d:%02d", alarm_start.hour, alarm_start.minute, alarm_end.hour, alarm_end.minute);

            update_screen();
        }

        button_pressed = -1;     // ready for next interrupt
    }
}

/**
 * Main
 */
void app_main(void)
{
    // Init I2C & MPU
    ESP_LOGI(TAG, "Initializing I2C and MPU...");
    ESP_ERROR_CHECK(nvs_flash_init());
    i2c_master_init_TUWEL();
    init_mpu6886();
    ESP_LOGI(TAG, "I2C and MPU initialized!");

    // init screen
    init_screen();
    render_screen();
    update_screen();


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
