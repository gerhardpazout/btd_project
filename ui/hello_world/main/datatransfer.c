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

#define BUFFER_FILE_PATH         "/spiffs/buffer.csv"
#define BUFFER_SEND_THRESHOLD    (10 * 1024)    // 10 KB
#define TRANSFER_CHECK_INTERVAL_MS  10000       // 10 seconds

#define SERVER_IP   "192.168.1.100"
#define SERVER_PORT 3333

#define START_MARKER "SENDING_DATA\n"
#define END_MARKER   "DATA_SENT\n"

static const char *TAG = "DataTransfer";

extern bool is_alarm_set;
extern bool wifi_is_connected(void);

void data_transfer_task(void *pvParameters) {
    bool prev_wifi_connected = wifi_is_connected();
    bool last_send_failed = false;

    char buffer[1024];  // 1 KB read chunk

    ESP_LOGI(TAG, "Data transfer task started.");

    for (;;) {
        if (!is_alarm_set) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        bool wifi = wifi_is_connected();
        struct stat st;
        size_t file_size = (stat(BUFFER_FILE_PATH, &st) == 0) ? st.st_size : 0;

        if (wifi && file_size > 0 &&
            (file_size >= BUFFER_SEND_THRESHOLD || (!prev_wifi_connected && wifi) || last_send_failed)) {

            ESP_LOGI(TAG, "Buffer size %d bytes – initiating data transfer", (int)file_size);

            int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if (sock < 0) {
                ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
            } else {
                struct sockaddr_in dest_addr = {
                    .sin_family = AF_INET,
                    .sin_port = htons(SERVER_PORT),
                    .sin_addr.s_addr = inet_addr(SERVER_IP),
                };
                memset(dest_addr.sin_zero, 0, sizeof(dest_addr.sin_zero));

                if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
                    ESP_LOGE(TAG, "Connection to server failed: errno %d", errno);
                    last_send_failed = true;
                    close(sock);
                } else {
                    ESP_LOGI(TAG, "Connected to server, sending START_MARKER...");
                    bool send_error = false;

                    if (send(sock, START_MARKER, strlen(START_MARKER), 0) < 0) {
                        ESP_LOGE(TAG, "Failed to send START_MARKER");
                        send_error = true;
                    }

                    FILE *file = NULL;
                    if (!send_error) {
                        file = fopen(BUFFER_FILE_PATH, "r");
                        if (!file) {
                            ESP_LOGE(TAG, "Failed to open buffer file");
                            send_error = true;
                        }
                    }

                    if (!send_error && file) {
                        ESP_LOGI(TAG, "Sending CSV content...");
                        while (!feof(file)) {
                            size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
                            if (bytes_read == 0) break;
                            const char *ptr = buffer;
                            ssize_t to_send = bytes_read;
                            while (to_send > 0) {
                                ssize_t sent = send(sock, ptr, to_send, 0);
                                if (sent < 0) {
                                    ESP_LOGE(TAG, "Error during send (errno %d)", errno);
                                    send_error = true;
                                    break;
                                }
                                to_send -= sent;
                                ptr += sent;
                            }
                            if (send_error) break;
                        }
                        fclose(file);
                    }

                    if (!send_error) {
                        ESP_LOGI(TAG, "Sending END_MARKER...");
                        if (send(sock, END_MARKER, strlen(END_MARKER), 0) < 0) {
                            ESP_LOGE(TAG, "Failed to send END_MARKER");
                            send_error = true;
                        }
                    }

                    // Optional: receive ACK
                    if (!send_error) {
                        char ack[16] = {0};
                        int len = recv(sock, ack, sizeof(ack) - 1, 0);
                        if (len > 0) {
                            ESP_LOGI(TAG, "Server ACK: %s", ack);
                        } else {
                            ESP_LOGW(TAG, "No ACK from server");
                        }
                    }

                    shutdown(sock, 0);
                    close(sock);

                    if (send_error) {
                        last_send_failed = true;
                        ESP_LOGW(TAG, "Send failed – will retry later.");
                    } else {
                        ESP_LOGI(TAG, "Upload complete. Clearing buffer.");
                        last_send_failed = false;
                        if (remove(BUFFER_FILE_PATH) != 0) {
                            ESP_LOGW(TAG, "Could not delete buffer file. Truncating instead.");
                            FILE *fclear = fopen(BUFFER_FILE_PATH, "w");
                            if (fclear) fclose(fclear);
                        }
                    }
                }
            }
        }

        prev_wifi_connected = wifi;
        vTaskDelay(TRANSFER_CHECK_INTERVAL_MS / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}
