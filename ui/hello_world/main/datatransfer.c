// datatransfer.c
/*
#include "datatransfer.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#define SERVER_IP   "192.168.1.100"
#define SERVER_PORT 3333
#define CHUNK       1024
#define CSV_PATH    "/spiffs/buffer.csv"
#define TAG         "DATATRANSFER"

static int send_all(int sock, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int res = send(sock, buf + sent, len - sent, 0);
        if (res <= 0) return -1;
        sent += res;
    }
    return 0;
}

void datatransfer_task(void *arg) {
    while (1) {
        struct stat st;
        if (stat(CSV_PATH, &st) != 0 || st.st_size < 128) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        FILE *f = fopen(CSV_PATH, "rb");
        if (!f) {
            ESP_LOGE(TAG, "Failed to open file for reading");
            vTaskDelay(pdMS_TO_TICKS(30000));
            continue;
        }

        struct sockaddr_in server = {
            .sin_family = AF_INET,
            .sin_port = htons(SERVER_PORT),
            .sin_addr.s_addr = inet_addr(SERVER_IP)
        };

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
            ESP_LOGE(TAG, "Server connect failed");
            fclose(f);
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(30000));
            continue;
        }

        send_all(sock, "SENDING_DATA\n", 13);
        char buf[CHUNK];
        size_t len;
        while ((len = fread(buf, 1, CHUNK, f)) > 0) {
            if (send_all(sock, buf, len) != 0) break;
        }
        send_all(sock, "DATA_SENT\n", 10);

        recv(sock, buf, sizeof(buf), 0); // Optional: receive server OK
        fclose(f);
        close(sock);
        unlink(CSV_PATH);
        ESP_LOGI(TAG, "Data sent and local file cleared");

        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
*/