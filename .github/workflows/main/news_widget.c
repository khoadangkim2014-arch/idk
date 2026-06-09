#include "news_widget.h"
#include "display.h"
#include "battery.h"
#include "board_config.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "NEWS";
#define MAX_HEADLINES 5
#define HEADLINE_LEN  80

static char s_headlines[MAX_HEADLINES][HEADLINE_LEN];
static int  s_count = 0;
static int  s_current = 0;
static int  s_scroll = 0;

static char s_resp_buf[4096];
static int  s_resp_len = 0;

static esp_err_t http_event(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0) {
        int space = (int)sizeof(s_resp_buf) - s_resp_len - 1;
        int copy  = evt->data_len < space ? evt->data_len : space;
        memcpy(s_resp_buf + s_resp_len, evt->data, copy);
        s_resp_len += copy;
        s_resp_buf[s_resp_len] = '\0';
    }
    return ESP_OK;
}

static void fetch_task(void *arg) {
    while (1) {
        s_resp_len = 0;
        memset(s_resp_buf, 0, sizeof(s_resp_buf));

        esp_http_client_config_t cfg = {
            .url            = NEWS_API_URL,
            .event_handler  = http_event,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };
        esp_http_client_handle_t client = esp_http_client_init(&cfg);
        esp_err_t ret = esp_http_client_perform(client);
        esp_http_client_cleanup(client);

        if (ret == ESP_OK && s_resp_len > 0) {
            cJSON *root = cJSON_Parse(s_resp_buf);
            if (root) {
                cJSON *articles = cJSON_GetObjectItem(root, "articles");
                int n = cJSON_GetArraySize(articles);
                if (n > MAX_HEADLINES) n = MAX_HEADLINES;
                s_count = 0;
                for (int i = 0; i < n; i++) {
                    cJSON *a = cJSON_GetArrayItem(articles, i);
                    cJSON *t = cJSON_GetObjectItem(a, "title");
                    if (t && t->valuestring) {
                        strlcpy(s_headlines[s_count], t->valuestring, HEADLINE_LEN);
                        s_count++;
                    }
                }
                cJSON_Delete(root);
                ESP_LOGI(TAG, "Fetched %d headlines", s_count);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(NEWS_REFRESH_SEC * 1000));
    }
}

void news_widget_init(void) {
    strlcpy(s_headlines[0], "Connecting...", HEADLINE_LEN);
    s_count = 1;
    xTaskCreate(fetch_task, "news_fetch", 8192, NULL, 3, NULL);
}

void news_widget_draw(void) {
    display_clear();

    /* Header */
    display_draw_rect(0, 0, 128, 10, true);
    display_set_cursor(2, 1);
    display_print("NEWS");
    display_draw_battery(battery_get_percent());

    if (s_count == 0) {
        display_set_cursor(0, 14);
        display_print("No headlines");
    } else {
        /* Cycle headline every 5 s (called ~10 Hz so 50 ticks) */
        static uint32_t tick = 0;
        if (++tick % 50 == 0) {
            s_current = (s_current + 1) % s_count;
            s_scroll  = 0;
        }
        display_set_cursor(0, 14);
        display_printf("%d/%d", s_current + 1, s_count);

        /* Scrolling headline text */
        display_draw_scrolling_text(s_headlines[s_current], 26, &s_scroll);

        /* Divider */
        display_draw_hline(0, 24, 128);
    }
    display_flush();
}
