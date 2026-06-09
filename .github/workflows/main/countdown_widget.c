/*
 * countdown_widget.c
 *
 * One-shot countdown timer widget.
 *   • Duration stored in NVS under key "cd_seconds" (uint32).
 *   • Set via idf.py menuconfig  →  "Dashboard Config" → "Countdown seconds"
 *     OR write directly with:
 *         nvs_set_u32(nvs, "cd_seconds", <value>); nvs_commit(nvs);
 *   • Display: large MM:SS centred on 128×64, with a thin progress bar.
 *   • States: IDLE → RUNNING → PAUSED → FINISHED
 *   • At zero: screen flashes 3× then stays on "DONE!".
 */

#include "countdown_widget.h"
#include "display.h"
#include "battery.h"
#include "board_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "COUNTDOWN";

/* ── State ───────────────────────────────────────────────── */
typedef enum { CD_IDLE, CD_RUNNING, CD_PAUSED, CD_FINISHED } cd_state_t;

static uint32_t   s_total_sec  = 0;   /* loaded from NVS          */
static uint32_t   s_remaining  = 0;   /* ticking value            */
static cd_state_t s_state      = CD_IDLE;
static int64_t    s_start_us   = 0;   /* esp_timer_get_time() snapshot */
static uint32_t   s_elapsed_at_pause = 0;

/* Flash effect on finish */
static uint8_t s_flash_count  = 0;
static bool    s_flash_on     = false;

/* ── Large digit font (8×16, digits 0-9 + ':') ───────────── */
/* Each glyph = 8 cols × 16 rows stored as 2 bytes per column (lo-row first) */
static const uint16_t large_font[][8] = {
    /* '0' */ {0x7FFE,0xFFFF,0x8181,0x8181,0x8181,0x8181,0xFFFF,0x7FFE},
    /* '1' */ {0x0000,0x8100,0xFFFF,0xFFFF,0x8000,0x0000,0x0000,0x0000},
    /* '2' */ {0xE1FE,0xF1FF,0x9981,0x9981,0x9981,0x9981,0x8FFF,0x87FE},
    /* '3' */ {0x4281,0xC3FF,0xDB99,0xDB99,0xDB99,0xDB99,0xFFFF,0x7E7E},
    /* '4' */ {0x0FFF,0x0FFF,0x0880,0x0880,0x0880,0x0880,0xFFFF,0xFFFF},
    /* '5' */ {0x47FF,0xC7FF,0xC399,0xC399,0xC399,0xC399,0xFF99,0x7E00},
    /* '6' */ {0x7FFE,0xFFFF,0xDB81,0xDB81,0xDB81,0xDB81,0xFF99,0x7E00},
    /* '7' */ {0x0001,0x0001,0xF001,0xF8FF,0x0CFF,0x0381,0x0181,0x0000},
    /* '8' */ {0x7EFE,0xFFFF,0xDB99,0xDB99,0xDB99,0xDB99,0xFFFF,0x7EFE},
    /* '9' */ {0x07FE,0x0FFF,0x9981,0x9981,0x9981,0x9981,0xFFFF,0x7FFE},
    /* ':' */ {0x0000,0x0000,0x1818,0x1818,0x0000,0x0000,0x0000,0x0000},
};

static void draw_large_char(int x, int y, char c) {
    int idx;
    if (c >= '0' && c <= '9') idx = c - '0';
    else if (c == ':')         idx = 10;
    else                       return;

    for (int col = 0; col < 8; col++) {
        uint16_t bits = large_font[idx][col];
        for (int row = 0; row < 16; row++)
            display_draw_pixel(x + col, y + row, (bits >> row) & 1);
    }
}

static void draw_large_string(int x, int y, const char *s) {
    while (*s) {
        draw_large_char(x, y, *s++);
        x += 9; /* 8px glyph + 1px gap */
    }
}

/* ── NVS helpers ─────────────────────────────────────────── */
static uint32_t load_from_nvs(void) {
    nvs_handle_t h;
    uint32_t val = COUNTDOWN_DEFAULT_SEC;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u32(h, "cd_seconds", &val);
        nvs_close(h);
    }
    ESP_LOGI(TAG, "Loaded %lu s from NVS", (unsigned long)val);
    return val;
}

/* ── Tick helper ─────────────────────────────────────────── */
static uint32_t elapsed_sec(void) {
    int64_t now = esp_timer_get_time();
    return (uint32_t)((now - s_start_us) / 1000000ULL);
}

/* ── Public API ──────────────────────────────────────────── */
void countdown_widget_init(void) {
    s_total_sec = load_from_nvs();
    s_remaining = s_total_sec;
    s_state     = CD_IDLE;
    ESP_LOGI(TAG, "Countdown widget ready: %lu s", (unsigned long)s_total_sec);
}

void countdown_start(void) {
    if (s_state == CD_FINISHED || s_state == CD_IDLE) {
        s_remaining = s_total_sec;
        s_elapsed_at_pause = 0;
    }
    s_start_us = esp_timer_get_time() - (int64_t)s_elapsed_at_pause * 1000000LL;
    s_state    = CD_RUNNING;
    s_flash_count = 0;
    ESP_LOGI(TAG, "Started: %lu s", (unsigned long)s_remaining);
}

void countdown_pause(void) {
    if (s_state != CD_RUNNING) return;
    s_elapsed_at_pause = elapsed_sec();
    s_state = CD_PAUSED;
}

void countdown_reset(void) {
    s_state            = CD_IDLE;
    s_remaining        = s_total_sec;
    s_elapsed_at_pause = 0;
    s_flash_count      = 0;
}

uint32_t countdown_remaining(void) { return s_remaining; }
bool     countdown_finished(void)  { return s_state == CD_FINISHED; }

/* ── Draw ────────────────────────────────────────────────── */
void countdown_widget_draw(void) {

    /* Update tick */
    if (s_state == CD_RUNNING) {
        uint32_t elapsed = elapsed_sec();
        if (elapsed >= s_total_sec) {
            s_remaining = 0;
            if (s_state != CD_FINISHED) {
                s_state       = CD_FINISHED;
                s_flash_count = 0;
                ESP_LOGI(TAG, "Countdown finished!");
            }
        } else {
            s_remaining = s_total_sec - elapsed;
        }
    }

    display_clear();

    /* ── Header ── */
    display_draw_rect(0, 0, 128, 10, true);
    display_set_cursor(2, 1);
    display_print("TIMER");
    display_draw_battery(battery_get_percent());

    /* ── Finished state: flash then show DONE ── */
    if (s_state == CD_FINISHED) {
        static uint8_t flash_tick = 0;
        if (s_flash_count < 6) {        /* 3 full flashes = 6 half-cycles */
            if (++flash_tick % 5 == 0) {
                s_flash_on = !s_flash_on;
                s_flash_count++;
            }
            if (s_flash_on) {
                display_draw_rect(0, 11, 128, 53, true); /* white fill */
            }
        } else {
            /* Steady DONE screen */
            display_set_cursor(28, 24);
            display_print("** DONE! **");
            display_set_cursor(14, 38);
            display_printf("00:00");
        }
        display_flush();
        return;
    }

    /* ── Large MM:SS centred ── */
    uint32_t mins = s_remaining / 60;
    uint32_t secs = s_remaining % 60;
    if (mins > 99) mins = 99;

    char timebuf[6];
    snprintf(timebuf, sizeof(timebuf), "%02lu:%02lu",
             (unsigned long)mins, (unsigned long)secs);

    /* 5 chars × 9px = 45px wide; centre on 128px → x = (128-45)/2 = 41 */
    draw_large_string(14, 14, timebuf);

    /* ── Progress bar (thin, below digits) ── */
    int pct = (s_total_sec > 0)
              ? (int)(s_remaining * 100 / s_total_sec)
              : 0;
    display_draw_progress_bar(4, 54, 120, 6, pct);

    /* ── State label bottom-left ── */
    display_set_cursor(2, 56);
    switch (s_state) {
        case CD_IDLE:    display_print("IDLE");   break;
        case CD_RUNNING: display_print("RUN");    break;
        case CD_PAUSED:  display_print("PAUSE");  break;
        default: break;
    }

    display_flush();
}
