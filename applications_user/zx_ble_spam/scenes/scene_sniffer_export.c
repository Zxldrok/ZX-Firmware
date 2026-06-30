#include "../zx_ble_spam.h"
#include "../nrf24.h"
#include "scenes.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>
#include <storage/storage.h>

static void button_callback(GuiButtonType type, InputType input_type, void* context) {
    (void)type; (void)input_type;
    App* app = context;
    if(!app->nrf24_connected) return;
    if(app->scan_count == 0) {
        app_add_log(app, "No data to export");
        return;
    }

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, EXT_PATH("ble_scan.csv"), FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, "MAC,RSSI,Channel,Payload\n", 24);
        for(uint8_t i = 0; i < app->scan_count; i++) {
            char line[128];
            ScanResult* p = &app->scan_results[i];
            int n = snprintf(line, sizeof(line), "%02X:%02X:%02X:%02X:%02X:%02X,%d,%d,",
                p->mac[0], p->mac[1], p->mac[2], p->mac[3], p->mac[4], p->mac[5],
                p->rssi, p->channel);
            for(uint8_t j = 0; j < p->payload_len && n < (int)sizeof(line) - 3; j++) {
                n += snprintf(line + n, sizeof(line) - n, "%02X", p->payload[j]);
            }
            line[n] = '\n';
            line[n+1] = '\0';
            storage_file_write(file, line, n+1);
        }
        storage_file_close(file);
        app_add_log(app, "Exported to ble_scan.csv");
    } else {
        app_add_log(app, "Failed to write file");
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventToggleScan);
}

void zx_ble_spam_scene_sniffer_export_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    if(!app->nrf24_connected) {
        widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "nRF24 not connected");
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return;
    }
    widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Sniffer Export");
    widget_add_string_element(app->widget, 64, 20, AlignCenter, AlignCenter, FontSecondary, "Export scan data");
    widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "to CSV on SD card");
    widget_add_button_element(app->widget, GuiButtonTypeCenter, "Export", button_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
}

bool zx_ble_spam_scene_sniffer_export_on_event(void* context, SceneManagerEvent event) {
    App* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventToggleScan) {
        text_box_set_text(app->text_box, app->log_text);
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewTextBox);
        return true;
    }
    return false;
}

void zx_ble_spam_scene_sniffer_export_on_exit(void* context) {
    App* app = context;
    text_box_set_text(app->text_box, "");
    widget_reset(app->widget);
}
