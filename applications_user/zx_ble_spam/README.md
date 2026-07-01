# Zx BLE Spam

Multi-tool BLE/nRF24 suite for Flipper Zero: scanner, spammer, sniffer, clone, and more.

## Features

| Mode | Description | Radio |
|------|-------------|-------|
| **Settings** | Configure BLE radio, advertising interval, TX power, MAC randomization | — |
| **BTLE Scanner** | Scan BLE advertising channels (37/38/39) | nRF24 |
| **FindMy Scanner** | Detect Apple AirTags and FindMy devices | nRF24 |
| **Peripheral Spam** | Spoof iBeacon/Apple/Samsung advertisements | nRF24 |
| **Sensor Reader** | Detect Xiaomi, HTS, ATC BLE sensors | nRF24 |
| **Device Clone** | Capture and replay BLE advertisements | nRF24 |
| **MouseJack** | Detect Logitech wireless mice (MouseJack vulnerability) | nRF24 |
| **Keyboard Spam** | Spoof BLE HID keyboard advertisements | nRF24 |
| **Remote Trigger** | Scan for FlipperPhone triggers | nRF24 |
| **Sniffer Export** | Export captured scan data to CSV on SD card | nRF24 |
| **Channel Scan** | Scan channels 0–79 for busiest channel | nRF24 |
| **BLE Spam** | Spam iOS/Android/Samsung/LG BLE devices via built-in STM32WB radio | Built-in |

## Requirements

- **Built-in BLE Spam** — no extra hardware required (uses STM32WB radio)
- **All other modes** — require nRF24 module wired to:
  - `PB2` → CE
  - `PA4` → CSN
  - `PB3` → SCK
  - `PA7` → MOSI
  - `PA6` → MISO
  - GND, 3.3V

## Build

```bash
./fbt fap_zx_ble_spam
./fbt launch APPSRC=zx_ble_spam
```

## Controls

- **OK** — Start/stop scan or spam
- **Back** — Return to menu
- **Left/Right** — Navigate menu (settings)
