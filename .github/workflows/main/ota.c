#include "ota.h"
#include "board_config.h"
#include "esp_https_ota.h"
#include "esp_log.h"

static const char *TAG = "OTA";

esp_err_t ota_check_and_update(void) {
    ESP_LOGI(TAG, "Checking for update at %s", OTA_UPDATE_URL);
    esp_http_client_config_t http_cfg = {
        .url               = OTA_UPDATE_URL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = true,
    };
    esp_https_ota_config_t ota_cfg = { .http_config = &http_cfg };
    esp_err_t ret = esp_https_ota(&ota_cfg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA success – rebooting");
        esp_restart();
    } else {
        ESP_LOGW(TAG, "OTA failed: %s", esp_err_to_name(ret));
    }
    return ret;
}
