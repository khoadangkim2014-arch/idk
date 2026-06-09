/*
 * battery.c
 *
 * Mock battery implementation for ESP32-S3 dashboard.
 *
 * ── To replace with a real ADC voltage divider ─────────────────────────────
 *   #include "esp_adc/adc_oneshot.h"
 *   Map raw ADC reading (e.g. GPIO1 = ADC1_CH0) to voltage, then to percent:
 *     float v    = (raw / 4095.0f) * 3.3f * DIVIDER_RATIO;
 *     int   pct  = (int)((v - VBAT_MIN) / (VBAT_MAX - VBAT_MIN) * 100);
 *
 * ── To replace with MAX17043 I²C fuel gauge ─────────────────────────────────
 *   Read register 0x04 over I²C (same bus as SSD1306):
 *     uint16_t raw; i2c_master_read_from_device(...);
 *     int pct = (raw >> 8);   // high byte = integer percent
 * ────────────────────────────────────────────────────────────────────────────
 */

#include "battery.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG  = "BATTERY";
static int  s_percent   = 85;   /* start at 85 % for demo */
static bool s_mock_init = false;

/* Background task: slowly counts down so you can watch the icon live */
static void mock_drain_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000)); /* drop 1% every 30 s */
        if (s_percent > 0) s_percent--;
        ESP_LOGD(TAG, "Mock battery: %d%%", s_percent);
    }
}

void battery_init(void) {
    if (s_mock_init) return;
    s_mock_init = true;
    ESP_LOGI(TAG, "Battery: mock mode (start %d%%)", s_percent);
    xTaskCreate(mock_drain_task, "batt_mock", 1024, NULL, 1, NULL);
}

int battery_get_percent(void) {
    /* ── REAL IMPLEMENTATION GOES HERE ── */
    return s_percent;
}
