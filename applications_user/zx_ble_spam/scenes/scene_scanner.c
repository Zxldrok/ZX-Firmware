#include "../zx_ble_spam.h"
#include "../nrf24.h"
#include "../views/scanner_view.h"
#include "scenes.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>

static const uint8_t ble_adv_channels[] = {37, 38, 39};
static FuriTimer* timer = NULL;

static void timer_callback(void* context) {
    App* app = context;
    uint8_t ch = ble_adv_channels[app->scan_channel % 3];
    app->scan_channel++;

    nrf24_set_channel(ch);
    nrf24_set_mode(NRF24ModeRx);
    nrf24_set_ble_adv_mode(true);

    NRF24Packet pkt;
    pkt.channel = ch;
    if(nrf24_receive_packet(&pkt, 20)) {
        app_scan_results_add(app, &pkt);
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
            pkt.mac[0], pkt.mac[1], pkt.mac[2], pkt.mac[3], pkt.mac[4], pkt.mac[5]);
        app_add_log(app, "CH%02d [%ddBm] %s", ch, pkt.rssi, mac_str);
    }
    scanner_view_update(app);
}

void zx_ble_spam_scene_scanner_on_enter(void* context) {
    App* app = context;
    if(!app->nrf24_connected) {
        widget_reset(app->widget);
        widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "nRF24 not connected");
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return;
    }

    app_scan_results_reset(app);
    app->log_text[0] = '\0';
    app->log_len = 0;
    app->scan_channel = 0;
    app->anim_frame = 0;
    nrf24_set_ble_adv_mode(true);

    timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, app);
    furi_timer_start(timer, 100);

    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewScanner);
}

bool zx_ble_spam_scene_scanner_on_event(void* context, SceneManagerEvent event) {
    App* app = context;
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == BLEEventUpdateDisplay) {
            return true;
        }
        if(event.event == BLEEventToggleScan) {
            if(timer) {
                furi_timer_free(timer);
                timer = NULL;
                nrf24_set_mode(NRF24ModeRx);
            } else {
                app_scan_results_reset(app);
                app->scan_channel = 0;
                nrf24_set_ble_adv_mode(true);
                timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, app);
                furi_timer_start(timer, 100);
            }
            scanner_view_update(app);
            return true;
        }
    }
    return false;
}

void zx_ble_spam_scene_scanner_on_exit(void* context) {
    if(timer) { furi_timer_free(timer); timer = NULL; }
    App* app = context;
    nrf24_set_ble_adv_mode(false);
    text_box_set_text(app->text_box, "");
    widget_reset(app->widget);
}
