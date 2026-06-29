<h3 align="center">
    <a href="https://github.com/Zxldrok/zx_firmware">
        <img src="https://github.com/user-attachments/assets/466c40d5-f6a1-444d-a235-d9026f7cd0ff" align="center" alt="Zx Firmware Logo" border="0">  
    </a>
</h3>

# Zx Firmware

Custom firmware for Flipper Zero, based on [Unleashed Firmware](https://github.com/DarkFlippers/unleashed-firmware).

> [!WARNING]
> This software is intended solely for experimental purposes and is not meant for any illegal activities.
> We do not condone unlawful behavior and strongly encourage you to use it only within the bounds of the law.
>
> This project is developed independently and is not affiliated with Flipper Devices.
>
> Also be aware, `Zxldrok/zx_firmware` is the only official page of the project, there is no paid, premium or closed source versions and if someone contacts you and say they are from our team and try to offer something like that - they are scammers, block that user ASAP

<br/>

## ZX Keylogger

Built-in BadUSB payload generator. Generates Duckyscript `.txt` files in `/ext/badusb/`, ready to run via the BadUSB app.

- **Windows Deploy** — PowerShell keylogger (`GetAsyncKeyState`) with window title tracking, shift+caps handling, full US symbol mapping. Logs to `%TEMP%\klog.txt`
- **Windows Get Log** — Opens the log file in Notepad on the target
- **Windows Cleanup** — Kills hidden PowerShell processes and deletes the log file
- **Linux Deploy** — Python3 stdin-based keylogger
- **macOS Deploy** — Python3 Quartz event-based keylogger (requires Accessibility permissions)

## IR Analyzer

Advanced IR signal analysis with turbo burst capture, waveform display, save and delete.

## ZX Signal Scope

Real-time Sub-GHz RSSI waveform analyzer. Displays a scrolling amplitude graph on the Flipper's screen.

- **4 frequency bands** — 315 MHz, 433 MHz, 868 MHz, 915 MHz (switch with Left/Right)
- **Threshold line** — adjustable with Up/Down
- **Peak hold** — dots show max amplitude per column
- **Freeze mode** — press OK to pause, inspect captured waveform
- **Live RSSI readout** — current dBm value displayed in status bar

## Build FAP apps

```bash
# With ufbt (recommended for standalone FAP development)
pip install --upgrade ufbt
cd applications_user/zx_signal_scope
ufbt build APPID=zx_signal_scope

# With fbt (full firmware toolchain)
./fbt fap_zx_signal_scope
./fbt launch APPSRC=zx_signal_scope     # build + upload + launch over USB
```

## Build (full firmware)

```bash
pip install -r scripts/requirements.txt
./fbt flash_usb
```
