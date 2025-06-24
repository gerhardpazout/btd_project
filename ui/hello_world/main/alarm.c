#include "alarm.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utility.h"

#define BUZZER_PIN 2  // adjust this to match your ESP hardware
#define TAG "ALARM"

static TaskHandle_t alarm_task_handle = NULL;

void alarm_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .freq_hz          = 2000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num       = BUZZER_PIN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&channel);
}

void beep(int duration_ms) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4000);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void alarm_task(void *arg) {
    int64_t target_ts = *(int64_t *)arg;
    free(arg);  // Clean up allocated memory

    ESP_LOGI(TAG, "Alarm task started. Waiting for target timestamp %lld", target_ts);

    while (1) {
        int64_t now = now_ms();
        if (now >= target_ts) {
            ESP_LOGI(TAG, "Alarm time reached (%lld ms) — triggering beep!", now);
            beep(500);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // check every 100ms
    }

    alarm_task_handle = NULL;
    vTaskDelete(NULL);  // Clean up this task
}

void startAlarmAt(int64_t timestamp_ms) {
    ESP_LOGI("ALARM", "startAlarmAt() called");

    if (alarm_task_handle != NULL) {
        ESP_LOGW(TAG, "Alarm already scheduled.");
        return;
    }

    int64_t *ts_copy = malloc(sizeof(int64_t));
    if (!ts_copy) {
        ESP_LOGE(TAG, "Failed to allocate memory for timestamp");
        return;
    }
    *ts_copy = timestamp_ms;

    xTaskCreate(alarm_task, "alarm_task", 2048, ts_copy, 5, &alarm_task_handle);
}
