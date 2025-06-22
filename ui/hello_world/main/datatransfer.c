// ── data_transfer_task.c ─────────────────────────────────────────
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "shared_globals.h"

#define BUFFER_FILE_PATH            "/spiffs/buffer.csv"
#define BUFFER_SEND_THRESHOLD       (10 * 1024)   // minimum size of batch before sending
#define TRANSFER_CHECK_INTERVAL_MS  10000        // every 10 s

#define SERVER_IP   "192.168.1.100"
#define SERVER_PORT 3333
#define START_MARKER "SENDING_DATA\n"
#define END_MARKER   "DATA_SENT\n"

static const char *TAG = "DataTransfer";

// provided elsewhere
extern bool is_alarm_set;
extern bool wifi_is_connected(void);

static bool send_file_lines(int sock)
{
    FILE *f = fopen(BUFFER_FILE_PATH, "r");
    if (!f) {
        ESP_LOGE(TAG, "Cannot open %s", BUFFER_FILE_PATH);
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (send(sock, line, len, 0) < 0) {
            ESP_LOGE(TAG, "send() failed (errno %d)", errno);
            fclose(f);
            return false;
        }
    }
    fclose(f);
    return true;
}

void data_transfer_task(void *pv)
{
    bool last_send_failed = false;

    ESP_LOGI(TAG, "transfer task running…");

    while (1) {
        /* wait until user has set the alarm */
        if (!is_alarm_set) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP_LOGI(TAG, "Alarm is set!");
            continue;
        }

        /* only proceed if Wi-Fi is up and buffer file exists */
        if (!wifi_is_connected()) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            ESP_LOGI(TAG, "Wifi connection is set!");
            continue;
        }

        struct stat st;
        if (stat(BUFFER_FILE_PATH, &st) != 0) {
            /* file doesn’t exist yet → nothing to send */
            vTaskDelay(pdMS_TO_TICKS(2000));
            ESP_LOGI(TAG, "Buffer.csv exists!");
            continue;
        }

        size_t sz = st.st_size;
        if (sz < BUFFER_SEND_THRESHOLD && !last_send_failed) {
            /* not big enough and last try was OK → wait */
            vTaskDelay(pdMS_TO_TICKS(TRANSFER_CHECK_INTERVAL_MS));
            ESP_LOGI(TAG, "Buffer.csv big enough to be sent!");
            continue;
        }

        ESP_LOGI(TAG, "Trying to connect to socket...");

        /* ───── create socket and connect ───── */
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE(TAG, "socket() failed");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        struct sockaddr_in dest = {
            .sin_family = AF_INET,
            .sin_port   = htons(SERVER_PORT),
            .sin_addr.s_addr = inet_addr(SERVER_IP)
        };

        if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
            ESP_LOGE(TAG, "connect() failed (errno %d)", errno);
            close(sock);
            last_send_failed = true;
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        ESP_LOGI(TAG, "connected – sending batch (%d bytes)", (int)sz);

        bool ok = true;
        /* send start marker */
        ok &= send(sock, START_MARKER, strlen(START_MARKER), 0) >= 0;
        /* send file line-by-line */
        ok &= send_file_lines(sock);
        /* send end marker */
        if (ok)
            ok &= send(sock, END_MARKER, strlen(END_MARKER), 0) >= 0;

        shutdown(sock, 0);
        close(sock);

        /* server ack (optional) ignored here */

        if (ok) {
            ESP_LOGI(TAG, "batch sent – truncating buffer");
            FILE *fclear = fopen(BUFFER_FILE_PATH, "w");
            if (fclear) fclose(fclear);
            else ESP_LOGW(TAG, "Could not truncate buffer file");
            last_send_failed = false;
        } else {
            ESP_LOGW(TAG, "batch failed – will retry");
            last_send_failed = true;
        }

        ESP_LOGW(TAG, "trying to reconnect now to send next batch...");

        vTaskDelay(pdMS_TO_TICKS(TRANSFER_CHECK_INTERVAL_MS));
    }
}
