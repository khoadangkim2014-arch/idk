#include "music_widget.h"
#include "display.h"
#include "battery.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "MUSIC";

static char s_artist[48]  = "Unknown Artist";
static char s_title[48]   = "No Track";
static int  s_progress    = 0;
static int  s_scroll_a    = 0;
static int  s_scroll_t    = 0;

/* ── BLE AVRCP stub (wire up to NimBLE AVRCP callbacks) ── */
void music_widget_set_track(const char *artist, const char *title, int pct) {
    strlcpy(s_artist,   artist, sizeof(s_artist));
    strlcpy(s_title,    title,  sizeof(s_title));
    s_progress = pct;
    s_scroll_a = s_scroll_t = 0;
    ESP_LOGI(TAG, "Now playing: %s – %s (%d%%)", artist, title, pct);
}

void music_widget_init(void) {
    /* TODO: register NimBLE AVRCP target callbacks to call music_widget_set_track */
    ESP_LOGI(TAG, "Music widget ready (BLE AVRCP)");
}

void music_widget_draw(void) {
    display_clear();

    /* ♫ header bar */
    display_draw_rect(0, 0, 128, 10, true);
    display_set_cursor(2, 1);
    display_print("MUSIC");
    display_draw_battery(battery_get_percent());

    /* Music note icon (simple pixel art) */
    display_draw_pixel(112, 2, true); display_draw_pixel(113, 2, true);
    display_draw_pixel(114, 1, true); display_draw_pixel(114, 2, true);
    display_draw_pixel(114, 3, true); display_draw_pixel(114, 4, true);
    display_draw_pixel(112, 5, true); display_draw_pixel(113, 5, true);
    display_draw_pixel(112, 6, true); display_draw_pixel(113, 6, true);

    /* Artist (scrolling) */
    display_set_cursor(0, 13);
    display_print("By:");
    display_draw_scrolling_text(s_artist, 13, &s_scroll_a);

    /* Title (scrolling) */
    display_draw_scrolling_text(s_title, 24, &s_scroll_t);

    /* Progress bar */
    display_set_cursor(0, 36);
    display_printf("%3d%%", s_progress);
    display_draw_progress_bar(28, 38, 96, 6, s_progress);

    /* Play/pause icon */
    if (s_progress > 0 && s_progress < 100) {
        /* Triangle = play */
        for (int i = 0; i < 6; i++) {
            display_draw_vline(56 + i, 48 + i, 12 - 2 * i);
        }
    } else {
        /* Two bars = pause */
        display_draw_rect(53, 48, 4, 10, true);
        display_draw_rect(59, 48, 4, 10, true);
    }

    display_flush();
}
