#include "datacapture.h"
#include "imu.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CSV_PATH "/spiffs/buffer.csv"
#define SAMPLE_RATE_HZ 30
#define TAG "DATACAPTURE"

static void ensure_header(FILE *f) {
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0)
        fprintf(f, "timestamp,x,y,z,temp\n");
    rewind(f);
}

void print_csv_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE("READER", "Failed to open %s", path);
        return;
    }

    ESP_LOGI("READER", "Contents of %s:", path);
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);  // outputs over serial
    }
    fclose(f);
}

void datacapture_task(void *arg) {
    FILE *f = fopen(CSV_PATH, "a+");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", CSV_PATH);
        vTaskDelete(NULL);
    }
    ensure_header(f);

    const int delay_ms = 1000 / SAMPLE_RATE_HZ;
    while (1) {
        int64_t ts_ms = esp_timer_get_time() / 1000;
        float x, y, z;
        getAccelData(&x, &y, &z);
        float temp = 0.0f; // update once temperature sensor is added

        fprintf(f, "%lld,%.2f,%.2f,%.2f,%.2f\n", ts_ms, x, y, z, temp);
        fflush(f);

        ESP_LOGI(TAG, "writing data into CSV: %.2f,%.2f,%.2f,%.2f", x, y, z, temp);

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}