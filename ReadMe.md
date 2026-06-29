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

## 🚀 Usage

- **Installation Guide:** [HowToInstall.md](/documentation/HowToInstall.md)
- **FAQ:** [documentation/FAQ.md](/documentation/FAQ.md)
- **Official Docs:** [docs.flipper.net](https://docs.flipper.net)

<br/>

## 🆕 Custom Apps

### ZX Keylogger

A built-in BadUSB keylogger payload generator. Generates Duckyscript `.txt` files saved to `/ext/badusb/`, ready to run via the BadUSB app.

**Features:**
- **Windows Deploy** — Deploys a PowerShell keylogger via `GetAsyncKeyState` polling with:
  - Window title tracking (`GetForegroundWindow` / `GetWindowText`)
  - Shift + CapsLock support
  - Full US keyboard symbol mapping
  - Logs to `%TEMP%\klog.txt`
- **Windows Get Log** — Opens the log file in Notepad on the target
- **Windows Cleanup** — Kills hidden PowerShell processes and deletes the log file
- **Linux Deploy** — Python3 stdin-based keylogger via terminal
- **macOS Deploy** — Python3 Quartz event-based keylogger (requires Accessibility permissions)

### IR Analyzer

Advanced infrared signal analysis tool. Features:
- Turbo burst capture
- Waveform display
- Save and delete captured signals

<br/>

## 📦 Releases

Pre-built firmware releases are available on the [GitHub Releases](https://github.com/Zxldrok/zx_firmware/releases) page.

<br/>

## 👨‍💻 Development

### Prerequisites

- **Python 3.10+** with `pip`
- **Git**

### Building with ufbt (recommended for app development)

```bash
# Install ufbt
python -m pip install --upgrade ufbt

# Navigate to the app directory and build
cd applications_user/zx_keylogger
ufbt build APPID=zx_keylogger
```

### Building the full firmware with fbt

```bash
# Install required Python packages
pip install -r scripts/requirements.txt

# Build and flash
./fbt flash_usb
```

### Deploying FAP to Flipper over serial

Use the included deploy script:

```bash
cd applications_user/zx_keylogger
python deploy_fap.py
```

### Project structure

```
applications_user/    - Custom user applications
  zx_keylogger/       - ZX Keylogger app
  ir_analyzer/        - IR Analyzer app
applications/         - Built-in applications and services
assets/               - Assets used by applications and services
furi/                 - Furi Core: OS-level primitives and helpers
firmware/             - Firmware source code
lib/                  - Libraries and drivers
scripts/              - Supplementary scripts and Python libraries
```

### Other Documentation

- **Developer Docs:** [developer.flipper.net](https://developer.flipper.net/flipperzero/doxygen)
- **How to build firmware:** [HowToBuild.md](/documentation/HowToBuild.md)
- **Plugin Tutorial:** [English by DroomOne](https://github.com/DroomOne/Flipper-Plugin-Tutorial) | [Russian by Pavel Yakovlev](https://yakovlev.me/hello-flipper-zero)
- **Writing your own app:** [atmanos.com](https://flipper.atmanos.com/docs/overview/intro)

<br/>

## 🔗 Links

- Official Docs: [docs.flipper.net](https://docs.flipper.net)
- Official Forum: [forum.flipperzero.one](https://forum.flipperzero.one)
- Developer Documentation: [developer.flipper.net](https://developer.flipper.net/flipperzero/doxygen)
