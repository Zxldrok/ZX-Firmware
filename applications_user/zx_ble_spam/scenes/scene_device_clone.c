#include "../zx_ble_spam.h"
#include "../nrf24.h"
#include "scenes.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>

static FuriTimer* timer = NULL;
static bool captured = false;

static void timer_callback(void* context) {
    App* app = context;
    static uint8_t ch_idx = 0;
    const uint8_t chs[] = {37, 38, 39};

    if(captured) {
        nrf24_set_channel(chs[ch_idx % 3]);
        nrf24_set_mode(NRF24ModeTx);
        nrf24_set_ble_adv_mode(true);
        nrf24_send_packet(app->clone_buffer, app->clone_len, 20);
        nrf24_set_mode(NRF24ModeRx);
        ch_idx++;
        return;
    }

    uint8_t ch = chs[ch_idx % 3];
    ch_idx++;

    nrf24_set_channel(ch);
    nrf24_set_mode(NRF24ModeRx);
    nrf24_set_ble_adv_mode(true);

    NRF24Packet pkt;
    pkt.channel = ch;
    if(nrf24_receive_packet(&pkt, 30)) {
        app->clone_len = pkt.payload_len;
        memcpy(app->clone_buffer, pkt.payload, pkt.payload_len);
        captured = true;
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
            pkt.mac[0], pkt.mac[1], pkt.mac[2], pkt.mac[3], pkt.mac[4], pkt.mac[5]);
        app_add_log(app, "Captured %s CH%d", mac_str, ch);
        app_add_log(app, "Now replaying");
        view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventUpdateDisplay);
    }
}

static void button_callback(GuiButtonType type, InputType input_type, void* context) {
    (void)type; (void)input_type;
    App* app = context;
    if(timer) {
        furi_timer_free(timer); timer = NULL;
        captured = false;
        nrf24_set_ble_adv_mode(false);
    } else {
        nrf24_set_ble_adv_mode(true);
        captured = false;
        app->log_text[0] = '\0'; app->log_len = 0;
        app_add_log(app, "Listening for BLE adv");
        timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, app);
        furi_timer_start(timer, 100);
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventToggleScan);
}

void zx_ble_spam_scene_device_clone_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    if(!app->nrf24_connected) {
        widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "nRF24 not connected");
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return;
    }
    widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Device Clone");
    widget_add_string_element(app->widget, 64, 20, AlignCenter, AlignCenter, FontSecondary, "Captures and replays");
    widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "BLE advertisements");
    widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
}

bool zx_ble_spam_scene_device_clone_on_event(void* context, SceneManagerEvent event) {
    App* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventUpdateDisplay) {
        text_box_set_text(app->text_box, app->log_text);
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewTextBox);
        return true;
    }
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventToggleScan) {
        if(timer) {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Device Clone");
            widget_add_string_element(app->widget, 64, 25, AlignCenter, AlignCenter, FontSecondary, captured ? "Replaying" : "Listening");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Stop", button_callback, app);
        } else {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Device Clone");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return true;
    }
    return false;
}

void zx_ble_spam_scene_device_clone_on_exit(void* context) {
    if(timer) { furi_timer_free(timer); timer = NULL; }
    App* app = context;
    nrf24_set_ble_adv_mode(false);
    text_box_set_text(app->text_box, "");
    widget_reset(app->widget);
}
