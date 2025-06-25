#include "wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include <string.h>

#define WIFI_SSID "ssid"
#define WIFI_PASS "password"
#define MAX_RETRY 5

static const char *TAG = "wifi";
static EventGroupHandle_t wifi_evt_grp;
static bool wifi_connected = false;
static int retry_cnt = 0;

#define WIFI_CONNECTED_BIT BIT0

// Event handler
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *event_data) {
    switch (id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        wifi_connected = false;
        if (retry_cnt < MAX_RETRY) {
            esp_wifi_connect();
            retry_cnt++;
            ESP_LOGW(TAG, "reconnect... (%d)", retry_cnt);
        } else {
            ESP_LOGE(TAG, "reconnect failed");
        }
        break;

    case IP_EVENT_STA_GOT_IP: {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        retry_cnt = 0;
        wifi_connected = true;
        xEventGroupSetBits(wifi_evt_grp, WIFI_CONNECTED_BIT);
        break;
    }
    default:
        break;
    }
}

// Public helpers
bool wifi_is_connected(void) { 
    return wifi_connected;
}

// Init STA
void wifi_init_sta(void)
{
    // 1. NVS (required for WIFI)
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Net-stack & default event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // 3. WIFI driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 4. Event handlers
    wifi_evt_grp = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &wifi_event_handler,
                                               NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                               IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler,
                                               NULL));

    // 5. STA config
    wifi_config_t sta_cfg = { 0 };
    strncpy((char *)sta_cfg.sta.ssid, WIFI_SSID, sizeof(sta_cfg.sta.ssid));
    strncpy((char *)sta_cfg.sta.password, WIFI_PASS, sizeof(sta_cfg.sta.password));
    sta_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));

    // 6. Start and wait for first IP
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "connecting to \"%s\" …", WIFI_SSID);

    xEventGroupWaitBits(wifi_evt_grp,
                        WIFI_CONNECTED_BIT,
                        pdFALSE, pdFALSE,
                        portMAX_DELAY);

    ESP_LOGI(TAG, "WIFI ready");
}
