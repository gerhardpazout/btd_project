#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "BUTTON_ISR"

// GPIOs for Buttons
#define BUTTON_A_GPIO 37  // A = Orange front button
#define BUTTON_B_GPIO 39  // B = Side button (if available)

static TaskHandle_t button_task_handle = NULL;

// Use flags to tell task which button was pressed
volatile int button_pressed = -1;

// ISR Handler
static void IRAM_ATTR button_isr_handler(void* arg) {
    int pin = (int) arg;
    button_pressed = pin;
    xTaskNotifyFromISR(button_task_handle, 0, eNoAction, NULL);
}

// Task to handle button logic
void button_task(void* arg) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Debounce delay
        vTaskDelay(pdMS_TO_TICKS(50));

        if (button_pressed == BUTTON_A_GPIO && gpio_get_level(BUTTON_A_GPIO) == 0) {
            ESP_LOGI(TAG, "Button A pressed!");
            // TODO: Do something for A
        }
        else if (button_pressed == BUTTON_B_GPIO && gpio_get_level(BUTTON_B_GPIO) == 0) {
            ESP_LOGI(TAG, "Button B pressed!");
            // TODO: Do something for B
        }

        button_pressed = -1;
    }
}

void app_main(void) {
    // Configure both buttons
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = (1ULL << BUTTON_A_GPIO) | (1ULL << BUTTON_B_GPIO)
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_A_GPIO, button_isr_handler, (void*) BUTTON_A_GPIO);
    gpio_isr_handler_add(BUTTON_B_GPIO, button_isr_handler, (void*) BUTTON_B_GPIO);

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, &button_task_handle);

    ESP_LOGI(TAG, "Ready! Press Button A or B.");
}
