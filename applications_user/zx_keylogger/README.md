# ZX Keylogger

Generates BadUSB Duckyscript payloads for penetration testing engagements.

## Payloads

| Menu Item | File | Target |
|-----------|------|--------|
| Windows Deploy | `keylog_win_deploy.txt` | PowerShell `GetAsyncKeyState` keylogger with window title tracking |
| Windows Get Log | `keylog_win_retrieve.txt` | Opens log in Notepad on target |
| Windows Cleanup | `keylog_win_cleanup.txt` | Kills hidden PowerShell and deletes log |
| Linux Deploy | `keylog_linux_deploy.txt` | Python3 stdin-based keylogger |
| macOS Deploy | `keylog_macos_deploy.txt` | Python3 Quartz event-based keylogger (needs Accessibility permissions) |

## Usage

1. Open the app on Flipper Zero
2. Select a payload from the menu
3. Payload is saved to `/ext/badusb/<filename>.txt`
4. Plug Flipper Zero into the target as a USB keyboard
5. Open BadUSB app and run the saved payload

## Deployment Scripts

Python scripts in `tools/` help with firmware updates and FAP deployment:

```bash
# Deploy the app
python tools/deploy_fap.py --port COM3

# Flash firmware
python tools/flash_fw.py --port COM3 --fuf /ext/update/f7-update-local/update.fuf

# Check update file
python tools/check_update.py --port COM3
```

All scripts accept `--port`/`-p` (default: `COM5`).
