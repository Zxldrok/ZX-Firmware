#include "zx_ble_spam.h"
#include "scenes/scenes.h"
#include "views/scanner_view.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>
#include <toolbox/name_generator.h>

void app_add_log(App* app, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buf[256];
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    size_t len = strlen(buf);
    if(app->log_len + len + 2 > sizeof(app->log_text)) {
        char* newline = strchr(app->log_text, '\n');
        if(newline) {
            size_t remaining = strlen(newline + 1);
            memmove(app->log_text, newline + 1, remaining);
            app->log_text[remaining] = '\0';
            app->log_len = remaining;
        }
    }
    size_t current = strlen(app->log_text);
    snprintf(app->log_text + current, sizeof(app->log_text) - current, "%s\n", buf);
    app->log_len = strlen(app->log_text);
}

void app_scan_results_reset(App* app) {
    app->scan_count = 0;
    app->scan_channel = 0;
    memset(app->scan_results, 0, sizeof(app->scan_results));
}

void app_scan_results_add(App* app, NRF24Packet* pkt) {
    if(app->scan_count >= MAX_SCAN_RESULTS) return;
    ScanResult* sr = &app->scan_results[app->scan_count++];
    sr->channel = pkt->channel;
    sr->rssi = pkt->rssi;
    memcpy(sr->mac, pkt->mac, 6);
    sr->timestamp = furi_get_tick();
    sr->payload_len = pkt->payload_len;
    memcpy(sr->payload, pkt->payload, pkt->payload_len);
    sr->is_ble = pkt->is_ble;
}

static bool custom_event_callback(void* context, uint32_t event) {
    App* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool back_event_callback(void* context) {
    App* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static App* app_alloc(void) {
    App* app = malloc(sizeof(App));
    memset(app, 0, sizeof(App));

    app->gui = furi_record_open(RECORD_GUI);
    app->scene_manager = scene_manager_alloc(&zx_ble_spam_scene_handlers, app);
    app->view_dispatcher = view_dispatcher_alloc();

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, back_event_callback);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, AppViewSubmenu, submenu_get_view(app->submenu));

    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, AppViewWidget, widget_get_view(app->widget));

    app->text_box = text_box_alloc();
    view_dispatcher_add_view(app->view_dispatcher, AppViewTextBox, text_box_get_view(app->text_box));

    app->var_list = variable_item_list_alloc();
    view_dispatcher_add_view(app->view_dispatcher, AppViewVariableItemList, variable_item_list_get_view(app->var_list));

    app->popup = popup_alloc();
    view_dispatcher_add_view(app->view_dispatcher, AppViewPopup, popup_get_view(app->popup));

    app->scanner_view = scanner_view_alloc(app);
    view_dispatcher_add_view(app->view_dispatcher, AppViewScanner, app->scanner_view);

    return app;
}

static void app_free(App* app) {
    view_dispatcher_remove_view(app->view_dispatcher, AppViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, AppViewWidget);
    view_dispatcher_remove_view(app->view_dispatcher, AppViewTextBox);
    view_dispatcher_remove_view(app->view_dispatcher, AppViewVariableItemList);
    view_dispatcher_remove_view(app->view_dispatcher, AppViewPopup);
    view_dispatcher_remove_view(app->view_dispatcher, AppViewScanner);

    submenu_free(app->submenu);
    view_free(app->scanner_view);
    widget_free(app->widget);
    text_box_free(app->text_box);
    variable_item_list_free(app->var_list);
    popup_free(app->popup);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);
}

int32_t zx_ble_spam_app_entry(void* p) {
    UNUSED(p);
    App* app = app_alloc();

    app->nrf24_connected = nrf24_init();
    if(app->nrf24_connected) {
        app->nrf24_connected = nrf24_check_connection();
    }

    scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneMenu);
    view_dispatcher_run(app->view_dispatcher);

    if(app->nrf24_connected) {
        nrf24_set_mode(NRF24ModeRx);
        nrf24_deinit();
    }

    app_free(app);
    return 0;
}
