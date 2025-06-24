#include "datacapture.h"
#include "imu.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "shared_globals.h"
#include <sys/time.h>
#include "utility.h"



#define CSV_PATH "/spiffs/buffer.csv"
#define SAMPLE_RATE_HZ 30
#define TAG "DATACAPTURE"


// constants for temperature sensor:
/**************** I2C CONFIG ****************/
//#define I2C_MASTER_SCL_IO 26
// #define I2C_MASTER_SDA_IO 0
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

#define I2C_PORT I2C_NUM_1
#define SDA_PIN 0
#define SCL_PIN 26

#define SHT30_SCL_IO 26
#define SHT30_SDA_IO 0
#define SHT30_PORT   I2C_NUM_1 

#define SHT30_ADDR 0x44

void log_timestamp_readable(int64_t ts_ms)
{
    time_t seconds = ts_ms / 1000;
    struct tm timeinfo;
    localtime_r(&seconds, &timeinfo);

    char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);

    ESP_LOGI(TAG, "Timestamp: %lld ms (%s)", ts_ms, time_str);
}

static void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = SHT30_PORT,
        .sda_io_num = SHT30_SDA_IO,
        .scl_io_num = SHT30_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err;

    err = i2c_param_config(SHT30_PORT, &conf);
    if (err == ESP_OK) {
        ESP_LOGI("I2C_INIT", "i2c_param_config: OK");
    } else {
        ESP_LOGE("I2C_INIT", "i2c_param_config FAILED: %s", esp_err_to_name(err));
    }

    err = i2c_driver_install(SHT30_PORT,
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
    return i2c_master_write_to_device(SHT30_PORT, addr, data, len, pdMS_TO_TICKS(100));
}
static esp_err_t i2c_read(uint8_t addr, uint8_t *data, size_t len)
{
    return i2c_master_read_from_device(SHT30_PORT, addr, data, len, pdMS_TO_TICKS(100));
}

/**************** SHT30 (Temp+RH) ****************/
static esp_err_t sht30_sample_raw(uint16_t *raw_t, uint16_t *raw_rh)
{
    /* Single‑shot high‑repeatability command */
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

    err = i2c_master_write_to_device(SHT30_PORT, SHT30_ADDR, cmd, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(20)); // Measurement delay

    err = i2c_master_read_from_device(SHT30_PORT, SHT30_ADDR, data, 6, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(err));
        return err;
    }

    uint16_t raw_temp = (data[0] << 8) | data[1];
    *temp_c = -45.0f + 175.0f * ((float)raw_temp / 65535.0f);

    return ESP_OK;
}

/**************** QMP6988 (Pressure) ****************/
#define QMP_ADDR 0x70
/* Register addresses */
#define QMP6988_CHIP_ID     0xD1
#define QMP6988_PRESS_MSB   0xF7
/* The full compensation routine needs calibration registers;   
 * for brevity, we perform a quick uncompensated read and return raw pressure in Pa.
 * For accurate results, port the full driver from QST.
 */
static int32_t qmp6988_read_raw_press(void)
{
    uint8_t buf[3];
    if (i2c_read(QMP_ADDR, buf, 3) == ESP_OK) {
        return (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);
    }
    return -1;
}

void i2c_scan2() {
    ESP_LOGI("I2C", "Scanning for devices on I2C bus...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI("I2C", "Found device at address 0x%02X", addr);
        }
    }
    ESP_LOGI("I2C", "Scan complete.");
}

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
    i2c_master_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    i2c_scan2();

    FILE *f = fopen(CSV_PATH, "a+");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", CSV_PATH);
        vTaskDelete(NULL);
    }
    ensure_header(f);

    const int delay_ms = 1000 / SAMPLE_RATE_HZ;
    while (1) {
        // Only start sampling if alarm is set
        if (!is_alarm_set) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        int64_t ts_ms = now_ms();
        // log_timestamp_readable(ts_ms);

        float x, y, z;
        getAccelData(&x, &y, &z);

        float temp = 0.0f; // update once temperature sensor is added
        sht30_read_temp(&temp);

        fprintf(f, "%lld,%.2f,%.2f,%.2f,%.2f\n", ts_ms, x, y, z, temp);
        fflush(f);

        // ESP_LOGI(TAG, "writing data into CSV: %.2f,%.2f,%.2f,%.2f", x, y, z, temp);

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}