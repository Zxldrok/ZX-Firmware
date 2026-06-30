#pragma once
#include <gui/gui.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include <gui/modules/text_box.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/popup.h>
#include <gui/view.h>
#include "nrf24.h"
#include <extra_beacon.h>
#include <furi_hal_bt.h>

#define MAX_SCAN_RESULTS 128
#define MAX_LOG_LINES 20

typedef struct {
    uint8_t channel;
    int8_t rssi;
    uint8_t mac[6];
    uint32_t timestamp;
    uint8_t payload[32];
    uint8_t payload_len;
    bool is_ble;
    uint16_t count;
} ScanResult;

typedef enum {
    BLEModeScanner,
    BLEModeFindMy,
    BLEModePeripheralSpam,
    BLEModeSensorReader,
    BLEModeDeviceClone,
    BLEModeMouseJack,
    BLEModeKeyboardSpam,
    BLEModeFlipperPhone,
    BLEModeSnifferExport,
    BLEModeChannelScan,
    BLEModeBleSpam,

    BLEModeCount,
} BLEMode;

typedef enum {
    AppViewSubmenu,
    AppViewWidget,
    AppViewTextBox,
    AppViewVariableItemList,
    AppViewPopup,
    AppViewScanner,
} AppView;

typedef struct {
    Gui* gui;
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;

    Submenu* submenu;
    Widget* widget;
    TextBox* text_box;
    VariableItemList* var_list;
    Popup* popup;
    View* scanner_view;

    uint8_t anim_frame;
    BLEMode current_mode;
    bool nrf24_connected;

    bool ble_spam_active;
    uint32_t ble_spam_packets;
    uint8_t ble_spam_type;
    uint8_t ble_spam_mac[6];

    ScanResult scan_results[MAX_SCAN_RESULTS];
    uint16_t scan_count;
    uint8_t scan_channel;

    char log_text[2048];
    uint32_t log_len;

    char spam_payload[64];
    uint8_t spam_payload_len;
    uint8_t spam_channel;

    uint8_t clone_buffer[32];
    uint8_t clone_len;
} App;

typedef enum {
    BLEEventToggleScan = 100,
    BLEEventStopScan,
    BLEEventPacketReceived,
    BLEEventUpdateDisplay,
} AppCustomEvent;

void app_add_log(App* app, const char* format, ...);
void app_scan_results_reset(App* app);
void app_scan_results_add(App* app, NRF24Packet* pkt);
