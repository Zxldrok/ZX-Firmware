#include "../zx_ble_spam.h"
#include "../nrf24.h"
#include "scenes.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>

#define MOUSEJACK_CHANNELS 5
static const uint8_t mousejack_chs[MOUSEJACK_CHANNELS] = {2, 5, 27, 37, 51};

static void timer_callback(void* context) {
    App* app = context;
    uint8_t ch = mousejack_chs[app->scene_ch_idx % MOUSEJACK_CHANNELS];
    app->scene_ch_idx++;

    nrf24_set_channel(ch);
    nrf24_set_mode(NRF24ModeRx);

    NRF24Packet pkt;
    pkt.channel = ch;
    if(nrf24_receive_packet(&pkt, 15)) {
        bool mouse_sig = false;
        for(uint8_t i = 0; i < pkt.payload_len - 3; i++) {
            if(pkt.payload[i] == 0x00 && pkt.payload[i+1] == 0x01 && pkt.payload[i+2] == 0x00) {
                mouse_sig = true;
                break;
            }
        }
        if(mouse_sig) {
            app->scene_count++;
            app_add_log(app, "MJ CH%d: %d pkt", ch, pkt.payload_len);
            view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventUpdateDisplay);
        }
    }
}

static void button_callback(GuiButtonType type, InputType input_type, void* context) {
    (void)type; (void)input_type;
    App* app = context;
    if(app->scene_timer) {
        furi_timer_free(app->scene_timer); app->scene_timer = NULL;
    } else {
        app->log_text[0] = '\0'; app->log_len = 0;
        app->scene_count = 0; app->scene_ch_idx = 0;
        app_add_log(app, "MouseJack scan started");
        app->scene_timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, app);
        furi_timer_start(app->scene_timer, 100);
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventToggleScan);
}

void zx_ble_spam_scene_mousejack_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    if(!app->nrf24_connected) {
        widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "nRF24 not connected");
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return;
    }
    widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "MouseJack Scan");
    widget_add_string_element(app->widget, 64, 20, AlignCenter, AlignCenter, FontSecondary, "Detects Logitech");
    widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "wireless mice");
    widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
}

bool zx_ble_spam_scene_mousejack_on_event(void* context, SceneManagerEvent event) {
    App* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventUpdateDisplay) {
        text_box_set_text(app->text_box, app->log_text);
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewTextBox);
        return true;
    }
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventToggleScan) {
        if(app->scene_timer) {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "MouseJack Scan");
            widget_add_string_element(app->widget, 64, 25, AlignCenter, AlignCenter, FontSecondary, "Scanning");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Stop", button_callback, app);
        } else {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "MouseJack Scan");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return true;
    }
    return false;
}

void zx_ble_spam_scene_mousejack_on_exit(void* context) {
    App* app = context;
    if(app->scene_timer) { furi_timer_free(app->scene_timer); app->scene_timer = NULL; }
    text_box_set_text(app->text_box, "");
    widget_reset(app->widget);
}
