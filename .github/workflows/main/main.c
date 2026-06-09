/*
 * ESP32-S3 Dashboard Firmware
 * ───────────────────────────
 * Chip   : ESP32-S3 QFN56 rev 0.2
 * Flash  : 16 MB  (Manufacturer 0x85, Device 0x2018)
 * PSRAM  : 8 MB Octal AP_3v3
 * Crystal: 40 MHz  CPU: 240 MHz
 * MAC    : 98:88:e0:15:fb:18
 * Display: SSD1306 128×64 I²C
 * Features: Wi-Fi widget, News widget, Music widget, OTA
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "board_config.h"
#include "display.h"
#include "wifi_manager.h"
#include "news_widget.h"
#include "music_widget.h"
#include "battery.h"
#include "countdown_widget.h"
#include "ota.h"

static const char *TAG = "MAIN";

/* ── Widget IDs ──────────────────────────────────────────── */
typedef enum {
    WIDGET_WIFI = 0,
    WIDGET_NEWS,
    WIDGET_MUSIC,
    WIDGET_COUNTDOWN,
    WIDGET_COUNT
} widget_id_t;

static widget_id_t s_active = WIDGET_WIFI;
static TickType_t  s_last_switch;

/* ── Wi-Fi status widget (drawn inline) ──────────────────── */
static void draw_wifi_widget(void) {
    display_clear();

    /* Header */
    display_draw_rect(0, 0, 128, 10, true);
    display_set_cursor(2, 1);
    display_print("WI-FI");
    display_draw_battery(battery_get_percent());

    if (wifi_manager_is_connected()) {
        char ip[20];
        wifi_manager_get_ip(ip, sizeof(ip));
        int8_t rssi = wifi_manager_rssi();

        display_set_cursor(0, 14);
        display_print("Status: CONNECTED");

        display_set_cursor(0, 24);
        display_printf("IP: %s", ip);

        display_set_cursor(0, 34);
        display_printf("RSSI: %d dBm", rssi);

        /* Signal strength bars */
        int bars = (rssi > -55) ? 4 : (rssi > -67) ? 3 : (rssi > -78) ? 2 : 1;
        for (int i = 0; i < 4; i++) {
            int h = (i + 1) * 4;
            if (i < bars)
                display_draw_rect(100 + i * 7, 64 - h, 5, h, true);
            else
                display_draw_rect(100 + i * 7, 64 - h, 5, h, false);
        }

        display_set_cursor(0, 44);
        display_printf("MAC: 98:88:e0:15:fb:18");
    } else {
        display_set_cursor(0, 20);
        display_print("Not connected");
        display_set_cursor(0, 32);
        display_print("Check credentials");
    }

    display_flush();
}

/* ── Main loop task ──────────────────────────────────────── */
static void dashboard_task(void *arg) {
    s_last_switch = xTaskGetTickCount();

    while (1) {
        /* Auto-advance widgets */
        if (xTaskGetTickCount() - s_last_switch > pdMS_TO_TICKS(WIDGET_CYCLE_MS)) {
            s_active = (widget_id_t)((s_active + 1) % WIDGET_COUNT);
            s_last_switch = xTaskGetTickCount();
            ESP_LOGI(TAG, "Widget -> %d", s_active);
        }

        switch (s_active) {
            case WIDGET_WIFI:      draw_wifi_widget();        break;
            case WIDGET_NEWS:      news_widget_draw();        break;
            case WIDGET_MUSIC:     music_widget_draw();       break;
            case WIDGET_COUNTDOWN: countdown_widget_draw();   break;
            default: break;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); /* ~10 fps */
    }
}

/* ── OTA task (runs once on boot if Wi-Fi up) ────────────── */
static void ota_task(void *arg) {
    vTaskDelay(pdMS_TO_TICKS(5000)); /* wait for display init */
    ota_check_and_update();
    vTaskDelete(NULL);
}

/* ── app_main ────────────────────────────────────────────── */
void app_main(void) {
    ESP_LOGI(TAG, "ESP32-S3 Dashboard booting...");
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    /* NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Display */
    display_init();
    display_clear();
    display_set_cursor(10, 20);
    display_print("ESP32-S3");
    display_set_cursor(10, 32);
    display_print("Dashboard v1.0");
    display_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Wi-Fi */
    wifi_manager_init();

    /* Battery */
    battery_init();

    /* Widgets */
    news_widget_init();
    music_widget_init();
    countdown_widget_init();
    countdown_start();          /* auto-start on boot; remove if you prefer manual */

    /* Tasks */
    xTaskCreatePinnedToCore(dashboard_task, "dashboard", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(ota_task,       "ota",       8192, NULL, 2, NULL, 1);

    ESP_LOGI(TAG, "All tasks started. Free heap: %lu B", esp_get_free_heap_size());
}
