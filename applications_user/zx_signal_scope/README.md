# Zx Signal Scope

Real-time Sub-GHz RSSI waveform analyzer for Flipper Zero.

## Features

- **4 frequency bands** — 315 MHz, 433 MHz, 868 MHz, 915 MHz (Left/Right)
- **Threshold line** — adjustable with Up/Down
- **Peak hold** — dots show max amplitude per column
- **Freeze mode** — press OK to pause, inspect captured waveform
- **Live RSSI readout** — current dBm value displayed in status bar
- **Scrolling waveform** — real-time amplitude graph on the Flipper's screen

## Controls

| Key | Action |
|-----|--------|
| **OK** | Toggle freeze / unfreeze |
| **Left/Right** | Change frequency band |
| **Up/Down** | Adjust threshold line |
| **Long Back** | Exit app |

## Build

```bash
./fbt fap_zx_signal_scope
./fbt launch APPSRC=zx_signal_scope
```

## Technical Details

- Uses `furi_hal_subghz_get_rssi()` for signal sampling
- Samples at 20 Hz (50 ms interval)
- Displays 128 samples in a 128×64 pixel framebuffer
- RSSI range: −100 dBm to 0 dBm
