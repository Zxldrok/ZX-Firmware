#include "../zx_ble_spam.h"
#include "../nrf24.h"
#include "scenes.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>

static FuriTimer* timer = NULL;

static bool detect_airtag(NRF24Packet* pkt) {
    if(pkt->payload_len < 23) return false;
    for(uint8_t i = 0; i < pkt->payload_len - 1; i++) {
        if(pkt->payload[i] == 0xFF && pkt->payload[i+1] == 0x4C && i+3 < pkt->payload_len) {
            uint8_t type = pkt->payload[i+2];
            uint8_t company = pkt->payload[i+3];
            return (type == 0x00 && company == 0x12) ||
                   (type == 0x10 && company == 0x01) ||
                   (type == 0x10 && company == 0x02);
        }
    }
    return false;
}

static void timer_callback(void* context) {
    App* app = context;
    static uint8_t ch_idx = 0;
    const uint8_t chs[] = {37, 38, 39};
    uint8_t ch = chs[ch_idx % 3];
    ch_idx++;

    nrf24_set_channel(ch);
    nrf24_set_mode(NRF24ModeRx);
    nrf24_set_ble_adv_mode(true);

    NRF24Packet pkt;
    pkt.channel = ch;
    if(nrf24_receive_packet(&pkt, 30) && detect_airtag(&pkt)) {
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
            pkt.mac[0], pkt.mac[1], pkt.mac[2], pkt.mac[3], pkt.mac[4], pkt.mac[5]);
        app_add_log(app, "AirTag! [%ddBm] %s CH%d", pkt.rssi, mac_str, ch);
        app_scan_results_add(app, &pkt);
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventUpdateDisplay);
}

static void button_callback(GuiButtonType type, InputType input_type, void* context) {
    (void)type; (void)input_type;
    App* app = context;
    if(timer) {
        furi_timer_free(timer); timer = NULL;
        nrf24_set_ble_adv_mode(false);
    } else {
        nrf24_set_ble_adv_mode(true);
        app->log_text[0] = '\0'; app->log_len = 0;
        app->scan_count = 0;
        app_add_log(app, "FindMy scanning");
        timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, app);
        furi_timer_start(timer, 150);
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventToggleScan);
}

void zx_ble_spam_scene_findmy_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    if(!app->nrf24_connected) {
        widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "nRF24 not connected");
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return;
    }
    widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "FindMy Scanner");
    widget_add_string_element(app->widget, 64, 20, AlignCenter, AlignCenter, FontSecondary, "Detects AirTags");
    widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "and FindMy devices");
    widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
}

bool zx_ble_spam_scene_findmy_on_event(void* context, SceneManagerEvent event) {
    App* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventUpdateDisplay) {
        text_box_set_text(app->text_box, app->log_text);
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewTextBox);
        return true;
    }
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventToggleScan) {
        text_box_set_text(app->text_box, app->log_text);
        if(timer) {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "FindMy Scanner");
            widget_add_string_element(app->widget, 64, 25, AlignCenter, AlignCenter, FontSecondary, "Scanning");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Stop", button_callback, app);
        } else {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "FindMy Scanner");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return true;
    }
    return false;
}

void zx_ble_spam_scene_findmy_on_exit(void* context) {
    if(timer) { furi_timer_free(timer); timer = NULL; }
    App* app = context;
    nrf24_set_ble_adv_mode(false);
    text_box_set_text(app->text_box, "");
    widget_reset(app->widget);
}
