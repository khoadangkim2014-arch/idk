#pragma once
/* ═══════════════════════════════════════════════════════════
 *  Board: ESP32-S3 QFN56  rev 0.2
 *  Flash: 16 MB  |  PSRAM: 8 MB (Octal AP_3v3)
 *  Crystal: 40 MHz  |  CPU: 240 MHz  |  BT5 LE
 *  MAC: 98:88:e0:15:fb:18
 * ═══════════════════════════════════════════════════════════ */

/* ── Display (SSD1306 128×64 I²C) ── */
#define DISPLAY_WIDTH       128
#define DISPLAY_HEIGHT       64
#define DISPLAY_I2C_PORT    I2C_NUM_0
#define DISPLAY_I2C_ADDR    0x3C
#define DISPLAY_PIN_SDA     GPIO_NUM_8
#define DISPLAY_PIN_SCL     GPIO_NUM_9
#define DISPLAY_I2C_FREQ_HZ 400000

/* ── Wi-Fi ── */
#define WIFI_SSID_KEY       "wifi_ssid"
#define WIFI_PASS_KEY       "wifi_pass"
#define WIFI_MAX_RETRY      5

/* ── NVS namespace ── */
#define NVS_NAMESPACE       "dashboard"

/* ── OTA ── */
#define OTA_UPDATE_URL      "https://example.com/fw/esp32s3_dashboard.bin"

/* ── News API (newsapi.org) ── */
#define NEWS_API_URL        "https://newsapi.org/v2/top-headlines?country=us&apiKey=YOUR_KEY"
#define NEWS_REFRESH_SEC    300

/* ── Music (Last.fm or local metadata via BLE) ── */
#define MUSIC_REFRESH_SEC   5

/* ── Widget cycle period (ms) ── */
#define WIDGET_CYCLE_MS     8000

/* ── Countdown timer ── */
/* Default duration in seconds. Override by writing NVS key "cd_seconds".
 * Via serial:  nvs_set_u32(nvs, "cd_seconds", 600); nvs_commit(nvs);
 * Via menuconfig: set CONFIG_COUNTDOWN_DEFAULT_SEC in Kconfig.projbuild  */
#ifndef CONFIG_COUNTDOWN_DEFAULT_SEC
#define COUNTDOWN_DEFAULT_SEC  300u   /* 5 minutes */
#else
#define COUNTDOWN_DEFAULT_SEC  CONFIG_COUNTDOWN_DEFAULT_SEC
#endif
