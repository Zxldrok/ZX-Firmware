#include "../zx_ble_spam.h"
#include "scenes.h"

static const char* radio_options[] = {"Built-in", "External (nRF24)"};
static const char* interval_options[] = {"50ms", "100ms", "200ms", "500ms"};
static const char* power_options[] = {"-12dBm", "-8dBm", "-4dBm", "0dBm", "+3dBm", "+6dBm"};
static const char* cycle_options[] = {"1 pkt", "3 pkts", "5 pkts", "10 pkts"};

static void radio_cb(VariableItem* item) {
    App* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->settings_radio = idx;
    variable_item_set_current_value_text(item, radio_options[idx]);
    settings_save(app);
}

static void interval_cb(VariableItem* item) {
    App* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->settings_adv_interval = idx;
    variable_item_set_current_value_text(item, interval_options[idx]);
    settings_save(app);
}

static void power_cb(VariableItem* item) {
    App* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->settings_tx_power = idx;
    variable_item_set_current_value_text(item, power_options[idx]);
    settings_save(app);
}

static void mac_random_cb(VariableItem* item) {
    App* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->settings_mac_random = (idx == 1);
    variable_item_set_current_value_text(item, idx ? "On" : "Off");
    settings_save(app);
}

static void cycle_cb(VariableItem* item) {
    App* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->settings_cycle_rate = idx;
    variable_item_set_current_value_text(item, cycle_options[idx]);
    settings_save(app);
}

void zx_ble_spam_scene_settings_on_enter(void* context) {
    App* app = context;
    VariableItemList* list = app->var_list;
    variable_item_list_reset(list);

    VariableItem* item;

    item = variable_item_list_add(list, "BLE Radio", 2, radio_cb, app);
    variable_item_set_current_value_index(item, app->settings_radio);
    variable_item_set_current_value_text(item, radio_options[app->settings_radio]);

    item = variable_item_list_add(list, "Adv Interval", COUNT_OF(interval_options), interval_cb, app);
    variable_item_set_current_value_index(item, app->settings_adv_interval);
    variable_item_set_current_value_text(item, interval_options[app->settings_adv_interval]);

    item = variable_item_list_add(list, "TX Power", COUNT_OF(power_options), power_cb, app);
    variable_item_set_current_value_index(item, app->settings_tx_power);
    variable_item_set_current_value_text(item, power_options[app->settings_tx_power]);

    item = variable_item_list_add(list, "MAC Random", 2, mac_random_cb, app);
    variable_item_set_current_value_index(item, app->settings_mac_random ? 1 : 0);
    variable_item_set_current_value_text(item, app->settings_mac_random ? "On" : "Off");

    item = variable_item_list_add(list, "Device Cycle", COUNT_OF(cycle_options), cycle_cb, app);
    variable_item_set_current_value_index(item, app->settings_cycle_rate);
    variable_item_set_current_value_text(item, cycle_options[app->settings_cycle_rate]);

    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewVariableItemList);
}

bool zx_ble_spam_scene_settings_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void zx_ble_spam_scene_settings_on_exit(void* context) {
    App* app = context;
    variable_item_list_reset(app->var_list);
}
