#pragma once
#include <stdbool.h>
#include <stdint.h>

/**
 * countdown_widget_init()
 *   Loads duration from NVS (key "cd_seconds").
 *   Falls back to COUNTDOWN_DEFAULT_SEC from board_config.h.
 *   Call once in app_main after nvs_flash_init().
 */
void countdown_widget_init(void);

/**
 * countdown_widget_draw()
 *   Renders the countdown screen. Call ~10 Hz from dashboard task.
 */
void countdown_widget_draw(void);

/* Control API – call from anywhere (serial command, BLE, etc.) */
void countdown_start(void);
void countdown_pause(void);
void countdown_reset(void);

/** Returns remaining seconds */
uint32_t countdown_remaining(void);

/** Returns true if the timer has reached zero */
bool countdown_finished(void);
