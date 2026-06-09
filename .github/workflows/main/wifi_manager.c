#include "wifi_manager.h"
#include "board_config.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "WIFI";
static EventGroupHandle_t s_wifi_eg;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry = 0;
static esp_netif_t *s_netif = NULL;

static void event_handler(void *arg, esp_event_base_t base,
                          int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry++;
            ESP_LOGW(TAG, "Retry %d/%d", s_retry, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_eg, WIFI_FAIL_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        s_retry = 0;
        xEventGroupSetBits(s_wifi_eg, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_manager_init(void) {
    nvs_handle_t nvs;
    char ssid[64] = {0}, pass[64] = {0};
    size_t ssid_len = sizeof(ssid), pass_len = sizeof(pass);

    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (ret == ESP_OK) {
        nvs_get_str(nvs, WIFI_SSID_KEY, ssid, &ssid_len);
        nvs_get_str(nvs, WIFI_PASS_KEY, pass, &pass_len);
        nvs_close(nvs);
    }
    if (strlen(ssid) == 0) {
        /* Fallback: hard-coded credentials — replace or provision via BLE */
        strlcpy(ssid, "YourSSID", sizeof(ssid));
        strlcpy(pass, "YourPassword", sizeof(pass));
    }

    s_wifi_eg = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t ih_any, ih_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &ih_any));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &ih_ip));

    wifi_config_t wc = {};
    strlcpy((char *)wc.sta.ssid,     ssid, sizeof(wc.sta.ssid));
    strlcpy((char *)wc.sta.password, pass, sizeof(wc.sta.password));
    wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_eg,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE,
        pdMS_TO_TICKS(15000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to %s", ssid);
        return ESP_OK;
    }
    ESP_LOGW(TAG, "Failed to connect");
    return ESP_FAIL;
}

bool wifi_manager_is_connected(void) {
    return (xEventGroupGetBits(s_wifi_eg) & WIFI_CONNECTED_BIT) != 0;
}

void wifi_manager_get_ip(char *buf, size_t len) {
    esp_netif_ip_info_t info;
    if (esp_netif_get_ip_info(s_netif, &info) == ESP_OK)
        snprintf(buf, len, IPSTR, IP2STR(&info.ip));
    else
        strlcpy(buf, "0.0.0.0", len);
}

int8_t wifi_manager_rssi(void) {
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) return ap.rssi;
    return -127;
}
