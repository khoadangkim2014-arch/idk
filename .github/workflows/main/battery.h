#pragma once
#include <stdint.h>

/**
 * battery_init()
 *   Call once in app_main before using battery_get_percent().
 *   Currently uses a mock; swap in ADC or I²C fuel-gauge code later.
 */
void battery_init(void);

/**
 * battery_get_percent()
 *   Returns 0–100.
 *   Mock implementation simulates a slow discharge so you can see the
 *   icon change on screen without real hardware.
 *   Replace body with real ADC read or I²C fuel-gauge query (MAX17043 etc.)
 */
int battery_get_percent(void);
