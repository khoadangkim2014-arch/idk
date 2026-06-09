#pragma once
#include "esp_err.h"
#include <stdbool.h>

esp_err_t wifi_manager_init(void);
bool      wifi_manager_is_connected(void);
void      wifi_manager_get_ip(char *buf, size_t len);
int8_t    wifi_manager_rssi(void);
