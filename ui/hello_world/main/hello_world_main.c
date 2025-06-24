#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "time_simple.h"
#include "shared_globals.h"
#include <stdbool.h>
#include <time.h>

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

#include "wifi.h"
#include "datacapture.h"
#include "datatransfer.h"
#include "utility.h"

#define TAG "BUTTON_ISR"
#define TIMEZONE "CET-1CEST,M3.5.0/2,M10.5.0/3"



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
bool is_alarm_set = false;

void initialize_sntp(void)
{
    ESP_LOGI("SNTP", "Initializing SNTP...");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

void wait_for_time_sync(void)
{
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retries = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2020 - 1900) && ++retries < retry_count) {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d)", retries);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

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
    increase_alarm_index();
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

                is_alarm_set = true;

                ESP_LOGI(TAG, "Wakeup window set to (simpletime): %02d:%02d - %02d:%02d", alarm_start.hour, alarm_start.minute, alarm_end.hour, alarm_end.minute);

                ESP_LOGI(TAG, "Wakeup window set to (ts): %lld - %lld", time_simple_to_timestamp(alarm_start), time_simple_to_timestamp(alarm_end));

                int64_t alarm_start_ts = time_simple_to_timestamp(alarm_start);
                int64_t alarm_end_ts = time_simple_to_timestamp(alarm_end);

                char time1[16], time2[16];
                ts_to_hhmmss_str(alarm_start_ts, time1, sizeof(time1));
                ts_to_hhmmss_str(alarm_end_ts,   time2, sizeof(time2));

                ESP_LOGI(TAG, "Wakeup window set to (hh:mm): %s - %s", time1, time2);

                update_screen();
            } else {
                // first press: prepare one-shot timer
                waiting_second = true;
                xTimerStart(single_tmr, 0);          // 400 ms window
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
    // connect to wifi first
    wifi_init_sta();

    // set time
    setenv("TZ", TIMEZONE, 1);
    tzset();
    initialize_sntp();
    wait_for_time_sync();

    // Init I2C & MPU
    ESP_LOGI(TAG, "Initializing I2C and MPU...");
    ESP_ERROR_CHECK(nvs_flash_init());
    i2c_master_init_TUWEL();
    init_mpu6886();
    ESP_LOGI(TAG, "I2C and MPU initialized!");

    vTaskDelay(pdMS_TO_TICKS(100));

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

    ESP_LOGI(TAG, "Starting datacapture task...");
    xTaskCreate(datacapture_task, "datacapture", 4096, NULL, 5, NULL);

    BaseType_t result = xTaskCreate(
        data_transfer_task,       // Task function
        "DataTransferTask",       // Task name (for debugging)
        4096,                     // Stack size (in bytes)
        NULL,                     // Task parameter (not used here)
        tskIDLE_PRIORITY + 1,     // Priority (slightly above idle)
        NULL                      // Task handle (not needed here)
    );
    if (result != pdPASS) {
        ESP_LOGE("MAIN", "Failed to create DataTransferTask!");
    }

    // log contents of buffer.csv, just to make sure the code works!
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "Print contents of buffers.csv after 5 seconds (just to check)");
    print_csv_file("/spiffs/buffer.csv");



}
