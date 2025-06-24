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
#include "utility.h"

#define BUFFER_FILE_PATH            "/spiffs/buffer.csv"
#define BUFFER_SEND_THRESHOLD       (1 * 1024)   // minimum size of batch before sending
#define TRANSFER_CHECK_INTERVAL_MS  10000        // every 10 s

#define SERVER_IP   "192.168.1.100"
#define SERVER_PORT 3333
#define START_MARKER "SENDING_DATA\n"
#define END_MARKER   "DATA_SENT\n"
#define WAKE_UP_WINDOW_MARKER "WAKE_WINDOW"

static const char *TAG = "DataTransfer";

// provided elsewhere
// extern bool is_alarm_set;
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



void send_wakeup_timewindow() {
    ESP_LOGI(TAG, "send_wakeup_timewindow() called");

    bool wakeuptime_sent = false;

    while (!wakeuptime_sent) {
        if(is_alarm_set) {
            // int64_t alarm_start_ts = time_simple_to_timestamp(alarm_start);
            // int64_t alarm_end_ts = time_simple_to_timestamp(alarm_end);
            
            int64_t alarm_start_ts = now_ms() + 30 * 1000;
            int64_t alarm_end_ts = alarm_start_ts + 30 * 1000;

            char msg[64];
            snprintf(msg, sizeof(msg), "%s,%lld,%lld", WAKE_UP_WINDOW_MARKER, alarm_start_ts, alarm_end_ts);

            char time1[16], time2[16];
            ts_to_hhmmss_str(alarm_start_ts, time1, sizeof(time1));
            ts_to_hhmmss_str(alarm_end_ts,   time2, sizeof(time2));


            // prepare connection & connect
            if (!wifi_is_connected()) {
                vTaskDelay(pdMS_TO_TICKS(500));
                ESP_LOGI(TAG, "Wifi connection is set!");
                continue;
            }

            ESP_LOGI(TAG, "Trying to connect to socket to send wakeup time...");
            ESP_LOGI(TAG, "Trying to send wakup window of: %s - %s", time1 ,time2);

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
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }

            ESP_LOGI(TAG, "connected to socket – sending wakeuptime");

            bool ok = send(sock, msg, strlen(msg), 0) >= 0;

            if (ok) {
                char recv_buf[256] = {0};
                int len = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
                if (len > 0) {
                    recv_buf[len] = '\0';
                    int64_t ts = 0;
                    server_action_t action = parse_server_response(recv_buf, &ts);

                    switch (action) {
                        case ACTION_TRIGGER_ALARM:
                            ESP_LOGI("SERVER", "Triggering alarm at timestamp: %lld", ts);
                            // startAlarmAt(ts);
                            break;
                        case ACTION_NONE:
                            ESP_LOGI("SERVER", "No action in server response: %s", recv_buf);
                            break;
                        case ACTION_UNKNOWN:
                        default:
                            ESP_LOGW("SERVER", "Unknown action or malformed response: %s", recv_buf);
                            break;
                    }

                    wakeuptime_sent = true;
                } else {
                    ESP_LOGW(TAG, "No response from server after sending wake window.");
                }
            } else {
                ESP_LOGW(TAG, "Sending wakeup window failed!");
            }

            ////////////////////////
            /*
            bool ok = true;
            // send start marker
            ok = send(sock, msg, strlen(msg), 0) >= 0;
            // send file line-by-line

            shutdown(sock, 0);
            close(sock);

            // server ack (optional) ignored here

            if (ok) {
                ESP_LOGI(TAG, "wakeup window sent!");
                wakeuptime_sent = true;
                
            } else {
                ESP_LOGW(TAG, "sending wakeup window failed!");
            }
            */

            shutdown(sock, 0);
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else {
            ESP_LOGI(TAG, "Waiting for user to set wakeup window");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void data_transfer_task(void *pv)
{
    
    bool last_send_failed = false;

    ESP_LOGI(TAG, "transfer task running…");

    // send wakeup window first
    send_wakeup_timewindow();

    // send data "endlessly"
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
        // send start marker
        ok &= send(sock, START_MARKER, strlen(START_MARKER), 0) >= 0;
        //send file line-by-line
        ok &= send_file_lines(sock);
        //send end marker
        if (ok)
            ok &= send(sock, END_MARKER, strlen(END_MARKER), 0) >= 0;


        // server ack (optional) ignored here
        if (ok) {
            ESP_LOGI(TAG, "batch sent – truncating buffer");
            FILE *fclear = fopen(BUFFER_FILE_PATH, "w");
            if (fclear) fclose(fclear);
            else ESP_LOGW(TAG, "Could not truncate buffer file");
            last_send_failed = false;

            // process answer from server
            ESP_LOGI(TAG, "trying to process server answer...");

            char recv_buf[256] = {0};
            int len = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
            if (len > 0) {
                recv_buf[len] = '\0';
                int64_t ts = 0;
                server_action_t action = parse_server_response(recv_buf, &ts);

                ESP_LOGI("SERVER", "response from Server: %s", recv_buf);

                switch (action) {
                    case ACTION_TRIGGER_ALARM:
                        char alarm_time_str[16];
                        ts_to_hhmmss_str(ts, alarm_time_str, sizeof(alarm_time_str));
                        ESP_LOGI("SERVER", "Triggering alarm at timestamp: %lld (%s)", ts, alarm_time_str);
                        // startAlarmAt(ts);
                        break;
                    case ACTION_NONE:
                        ESP_LOGI("SERVER", "No action in server response: %s", recv_buf);
                        break;
                    case ACTION_UNKNOWN:
                    default:
                        ESP_LOGW("SERVER", "Unknown action or malformed response: %s", recv_buf);
                        break;
                }
            }

        } else {
            ESP_LOGW(TAG, "batch failed – will retry");
            last_send_failed = true;
        }
        

        shutdown(sock, 0);
        close(sock);

        ESP_LOGW(TAG, "trying to reconnect now to send next batch...");

        vTaskDelay(pdMS_TO_TICKS(TRANSFER_CHECK_INTERVAL_MS));
    }
}
