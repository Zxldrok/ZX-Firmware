#include "../zx_ble_spam.h"
#include "../nrf24.h"
#include "scenes.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>

static FuriTimer* timer = NULL;
static uint16_t ch_packets[128] = {0};
static uint8_t current_ch = 0;

static void timer_callback(void* context) {
    App* app = context;

    nrf24_set_channel(current_ch);
    nrf24_set_mode(NRF24ModeRx);
    nrf24_set_ble_adv_mode(false);

    NRF24Packet pkt;
    pkt.channel = current_ch;
    uint16_t count = 0;
    for(uint8_t i = 0; i < 5; i++) {
        if(nrf24_receive_packet(&pkt, 5)) count++;
    }
    ch_packets[current_ch] = count;
    current_ch = (current_ch + 1) % 80;

    if(current_ch == 0) {
        uint16_t total = 0;
        uint8_t max_ch = 0;
        uint16_t max_val = 0;
        for(uint8_t i = 0; i < 80; i++) {
            total += ch_packets[i];
            if(ch_packets[i] > max_val) { max_val = ch_packets[i]; max_ch = i; }
        }
        app_add_log(app, "Scan complete: %d pkt", total);
        app_add_log(app, "Busiest: CH%d (%d)", max_ch, max_val);
        view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventUpdateDisplay);
    }
}

static void button_callback(GuiButtonType type, InputType input_type, void* context) {
    (void)type; (void)input_type;
    App* app = context;
    if(timer) {
        furi_timer_free(timer); timer = NULL;
    } else {
        current_ch = 0;
        memset(ch_packets, 0, sizeof(ch_packets));
        app->log_text[0] = '\0'; app->log_len = 0;
        app_add_log(app, "Channel scan started");
        timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, app);
        furi_timer_start(timer, 30);
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventToggleScan);
}

void zx_ble_spam_scene_channel_scan_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    if(!app->nrf24_connected) {
        widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "nRF24 not connected");
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return;
    }
    widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Channel Scanner");
    widget_add_string_element(app->widget, 64, 20, AlignCenter, AlignCenter, FontSecondary, "Scans CH0-79");
    widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "Detects busiest");
    widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
}

bool zx_ble_spam_scene_channel_scan_on_event(void* context, SceneManagerEvent event) {
    App* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventUpdateDisplay) {
        text_box_set_text(app->text_box, app->log_text);
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewTextBox);
        return true;
    }
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventToggleScan) {
        if(timer) {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Channel Scanner");
            widget_add_string_element(app->widget, 64, 25, AlignCenter, AlignCenter, FontSecondary, "Scanning");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Stop", button_callback, app);
        } else {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Channel Scanner");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return true;
    }
    return false;
}

void zx_ble_spam_scene_channel_scan_on_exit(void* context) {
    if(timer) { furi_timer_free(timer); timer = NULL; }
    App* app = context;
    text_box_set_text(app->text_box, "");
    widget_reset(app->widget);
}
