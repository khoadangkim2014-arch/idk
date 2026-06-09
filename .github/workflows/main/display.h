#pragma once
#include <stdint.h>
#include <stdbool.h>

void display_init(void);
void display_clear(void);
void display_flush(void);

/* Primitives */
void display_draw_pixel(int x, int y, bool on);
void display_draw_hline(int x, int y, int len);
void display_draw_vline(int x, int y, int len);
void display_draw_rect(int x, int y, int w, int h, bool filled);

/* Text (built-in 6×8 font) */
void display_set_cursor(int x, int y);
void display_print(const char *str);
void display_printf(const char *fmt, ...);

/* Widget helpers */
void display_draw_scrolling_text(const char *text, int y, int *scroll_pos);
void display_draw_progress_bar(int x, int y, int w, int h, int percent);
void display_invert(bool on);

/* Battery icon – draws a 12×6 icon + "XX%" at top-right (x=104, y=2)
 * Call AFTER display_clear(), BEFORE display_flush().
 * percent: 0-100 */
void display_draw_battery(int percent);
