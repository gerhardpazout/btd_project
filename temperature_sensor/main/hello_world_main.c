/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include <math.h>
#include "driver/i2c.h"

#define TAG "SHT30"
#define VIB_GPIO 26

/**************** I2C CONFIG ****************/
/*
#define I2C_MASTER_SCL_IO 0
#define I2C_MASTER_SDA_IO 26
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

#define SHT30_ADDR 0x44

static void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err;

    err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err == ESP_OK) {
        ESP_LOGI("I2C_INIT", "i2c_param_config: OK");
    } else {
        ESP_LOGE("I2C_INIT", "i2c_param_config FAILED: %s", esp_err_to_name(err));
    }

    err = i2c_driver_install(I2C_MASTER_NUM,
                             conf.mode,
                             I2C_MASTER_RX_BUF_DISABLE,
                             I2C_MASTER_TX_BUF_DISABLE,
                             0);
    if (err == ESP_OK) {
        ESP_LOGI("I2C_INIT", "i2c_driver_install: OK");
    } else {
        ESP_LOGE("I2C_INIT", "i2c_driver_install FAILED: %s", esp_err_to_name(err));
    }
}

static esp_err_t i2c_write(uint8_t addr, const uint8_t *data, size_t len)
{
    return i2c_master_write_to_device(I2C_MASTER_NUM, addr, data, len, pdMS_TO_TICKS(100));
}
static esp_err_t i2c_read(uint8_t addr, uint8_t *data, size_t len)
{
    return i2c_master_read_from_device(I2C_MASTER_NUM, addr, data, len, pdMS_TO_TICKS(100));
}
*/

/**************** SHT30 (Temp+RH) ****************/
/*
static esp_err_t sht30_sample_raw(uint16_t *raw_t, uint16_t *raw_rh)
{
    // Single‑shot high‑repeatability command
    uint8_t cmd[2] = {0x2C, 0x06};
    esp_err_t err = i2c_write(SHT30_ADDR, cmd, 2);
    if (err != ESP_OK) {
        ESP_LOGE("SHT30", "I2C write failed: %s (%d)", esp_err_to_name(err), err);
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(15));
    uint8_t rx[6];
    ESP_ERROR_CHECK(i2c_read(SHT30_ADDR, rx, 6));
    *raw_t  = (rx[0] << 8) | rx[1];
    *raw_rh = (rx[3] << 8) | rx[4];
    return ESP_OK;
}
static void sht30_get(float *temp_c, float *rh)
{
    uint16_t rt, rrh;
    if (sht30_sample_raw(&rt, &rrh) == ESP_OK) {
        *temp_c = -45.0f + 175.0f * ((float)rt / 65535.0f);
        *rh     = 100.0f * ((float)rrh / 65535.0f);
    } else {
        *temp_c = NAN; *rh = NAN;
    }
}

static esp_err_t sht30_read_temp(float *temp_c) {
    uint8_t cmd[] = {0x2C, 0x06};  // High repeatability measurement
    uint8_t data[6];
    esp_err_t err;

    err = i2c_master_write_to_device(I2C_MASTER_NUM, SHT30_ADDR, cmd, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(20)); // Measurement delay

    err = i2c_master_read_from_device(I2C_MASTER_NUM, SHT30_ADDR, data, 6, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(err));
        return err;
    }

    uint16_t raw_temp = (data[0] << 8) | data[1];
    *temp_c = -45.0f + 175.0f * ((float)raw_temp / 65535.0f);

    return ESP_OK;
}
*/

/**************** QMP6988 (Pressure) ****************/
#define QMP_ADDR 0x70
// Register addresses */
#define QMP6988_CHIP_ID     0xD1
#define QMP6988_PRESS_MSB   0xF7
/* The full compensation routine needs calibration registers;   
 * for brevity, we perform a quick uncompensated read and return raw pressure in Pa.
 * For accurate results, port the full driver from QST.
 */
/*
static int32_t qmp6988_read_raw_press(void)
{
    uint8_t buf[3];
    if (i2c_read(QMP_ADDR, buf, 3) == ESP_OK) {
        return (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);
    }
    return -1;
}
*/

/**************** MAIN TASK ****************/
/*
static void env_task()
{
    printf("env_task()\n");
    float temp, hum;
    int loops = 3;
    //while (loops >= 0) {
        printf("while... \n");
        sht30_get(&temp, &hum);
        int32_t p_raw = qmp6988_read_raw_press();
        ESP_LOGI(TAG, "T=%.2f °C  RH=%.2f %%   P_raw=%ld", temp, hum, (long)p_raw);
        loops--;
        vTaskDelay(pdMS_TO_TICKS(2000));
    //}
}

void i2c_scan(void)
{
    ESP_LOGI("SCAN", "Scanning I2C bus...");
    uint8_t dummy = 0;
    bool found = false;

    for (uint8_t addr = 1; addr < 127; addr++) {
        esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, addr, &dummy, 1, pdMS_TO_TICKS(10));
        if (err == ESP_OK) {
            ESP_LOGI("SCAN", "✅ Found device at 0x%02X", addr);
            found = true;
        }
    }

    if (!found) {
        ESP_LOGW("SCAN", "X No I2C devices found. Double-check physical connections.");
    }

    ESP_LOGI("SCAN", "Scan complete");
    ESP_LOGI("SCAN", "SDA pin: %d, SCL pin: %d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);  // ← Add here
}

void i2c_scan_with_ack(void)
{
    ESP_LOGI("SCAN", "Scanning I2C bus with ACK check...");
    ESP_LOGI("SCAN", "SDA pin: %d, SCL pin: %d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    bool found = false;

    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);  // Send address with write bit
        i2c_master_stop(cmd);
        esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(10));
        i2c_cmd_link_delete(cmd);

        if (err == ESP_OK) {
            ESP_LOGI("SCAN", "✅ Found device at 0x%02X", addr);
            found = true;
        }
    }

    if (!found) {
        ESP_LOGW("SCAN", "❌ No I2C devices found. Still double-check wiring and pull-ups.");
    } else {
        ESP_LOGI("SCAN", "✅ Scan complete.");
    }
}
*/

void app_main(void)
{
    printf("Hello world!\n");

    gpio_reset_pin(VIB_GPIO);
    gpio_set_direction(VIB_GPIO, GPIO_MODE_OUTPUT);

    while (1) {
        ESP_LOGI("VIB", "Motor ON");
        gpio_set_level(VIB_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(300));

        ESP_LOGI("VIB", "Motor OFF");
        gpio_set_level(VIB_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    /*
    i2c_master_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    // i2c_scan();
    i2c_scan_with_ack();

    float temp;
    esp_err_t result = sht30_read_temp(&temp);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Temperature: %.2f°C", temp);
    } else {
        ESP_LOGE(TAG, "Failed to read temperature");
    }
    */

    /*
    i2c_scan();
    // xTaskCreate(env_task, "env_task", 4096, NULL, 5, NULL);
    env_task();
    */

    /* Print chip information */
    /*
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
    */
}
