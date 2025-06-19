/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BUTTON_A_PIN GPIO_NUM_37  
#define BUTTON_B_PIN GPIO_NUM_39

static const char *TAG = "UHRZEIT_EINSTELLUNG";

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_A_PIN) | (1ULL << BUTTON_B_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    int hour = 0;
    int previous_state_a = 1;
    int previous_state_b = 1;

    int double_click_counter = 0;
    int tick_counter = 0;
    const int max_ticks_for_double_click = 8; // ca. 400 ms

    bool waiting_for_second_click = false;

    ESP_LOGI(TAG, "Bereit: B = Stunde erhöhen, A = Einfachklick (Server starten), Doppelklick (Uhrzeit anzeigen)");

    while (1) {
        int current_state_a = gpio_get_level(BUTTON_A_PIN);
        int current_state_b = gpio_get_level(BUTTON_B_PIN);

        // Button B gedrückt: Stunde erhöhen 
        if (previous_state_b == 1 && current_state_b == 0) {
            hour = (hour + 1) % 24;
            ESP_LOGI(TAG, "Stunde eingestellt: %02d:00 Uhr", hour);
        }

        // Button A gedrückt 
        if (previous_state_a == 1 && current_state_a == 0) {
            if (waiting_for_second_click && tick_counter < max_ticks_for_double_click) {
                // Doppelklick erkannt
                ESP_LOGI(TAG, "Uhrzeit gespeichert: %02d:00 Uhr", hour);
                waiting_for_second_click = false;
                tick_counter = 0;
            } else {
                // Möglicher erster Klick
                waiting_for_second_click = true;
                tick_counter = 0;
            }
        }

        // Warte auf zweiten Klick (Doppelklick-Timeout prüfen)
        if (waiting_for_second_click) {
            tick_counter++;
            if (tick_counter > max_ticks_for_double_click) {
                // Kein zweiter Klick → Einfachklick erkannt
                ESP_LOGI(TAG, "Starte Server-Verbindung ...");
                waiting_for_second_click = false;
                tick_counter = 0;
            }
        }

        previous_state_a = current_state_a;
        previous_state_b = current_state_b;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
