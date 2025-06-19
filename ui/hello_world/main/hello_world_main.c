#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

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
