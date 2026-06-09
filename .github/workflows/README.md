# ESP32-S3 Dashboard Firmware

## Hardware spec
| Parameter       | Value                          |
|-----------------|--------------------------------|
| Chip            | ESP32-S3 QFN56 rev 0.2         |
| CPU             | Dual-core Xtensa LX7 @ 240 MHz |
| LP core         | Yes                            |
| Flash           | 16 MB (Mfr 0x85 Dev 0x2018)    |
| PSRAM           | 8 MB Octal AP_3v3              |
| Crystal         | 40 MHz                         |
| MAC             | 98:88:e0:15:fb:18              |
| Display         | SSD1306 128×64 I²C             |
| Wireless        | Wi-Fi 802.11 b/g/n + BT 5 LE  |

## Features
- **Wi-Fi widget** – IP, RSSI, signal bars, MAC
- **News widget** – live top headlines via newsapi.org (scrolling)
- **Music widget** – BLE AVRCP track metadata + progress bar
- **OTA** – HTTPS firmware update on boot

---

## Prerequisites
```
ESP-IDF v5.2+   https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/
```

## Quick start
```bash
# 1. Set target
idf.py set-target esp32s3

# 2. Configure (Wi-Fi creds, News API key, OTA URL)
idf.py menuconfig
#   → Example Connection Configuration → Wi-Fi SSID / Password
#   Or edit board_config.h directly

# 3. Build
idf.py build

# 4. Flash  (replace /dev/ttyUSB0 with your port)
idf.py -p /dev/ttyUSB0 flash monitor
```

### One-liner flash after build
```bash
esptool.py \
  --chip esp32s3 \
  --port /dev/ttyUSB0 \
  --baud 921600 \
  --before default_reset \
  --after hard_reset \
  write_flash \
    --flash_mode qio \
    --flash_freq 80m \
    --flash_size 16MB \
    0x0     build/bootloader/bootloader.bin \
    0x8000  build/partition_table/partition-table.bin \
    0xe000  build/ota_data_initial.bin \
    0x10000 build/esp32s3_dashboard.bin
```

## Pin assignments (SSD1306)
| Signal | GPIO |
|--------|------|
| SDA    | 8    |
| SCL    | 9    |

Change `DISPLAY_PIN_SDA` / `DISPLAY_PIN_SCL` in `board_config.h` to match your wiring.

## Customisation
| What              | Where                        |
|-------------------|------------------------------|
| Wi-Fi credentials | `board_config.h` or NVS      |
| News API key      | `NEWS_API_URL` in board_config.h |
| OTA URL           | `OTA_UPDATE_URL` in board_config.h |
| Widget cycle time | `WIDGET_CYCLE_MS`            |
| I²C pins          | `DISPLAY_PIN_SDA/SCL`        |
| AVRCP callbacks   | `music_widget.c`             |
