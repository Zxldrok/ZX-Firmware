#include "../zx_ble_spam.h"
#include "../nrf24.h"
#include "scenes.h"

static void menu_callback(void* context, uint32_t index) {
    App* app = context;

    switch(index) {
    case 0:  scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneScanner); break;
    case 1:  scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneFindMy); break;
    case 2:  scene_manager_next_scene(app->scene_manager, ZxBleSpamScenePeripheralSpam); break;
    case 3:  scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneSensorReader); break;
    case 4:  scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneDeviceClone); break;
    case 5:  scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneMouseJack); break;
    case 6:  scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneKeyboardSpam); break;
    case 7:  scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneFlipperPhone); break;
    case 8:  scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneSnifferExport); break;
    case 9:  scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneChannelScan); break;
    case 10: scene_manager_next_scene(app->scene_manager, ZxBleSpamSceneBleSpam); break;
    default: return;
    }
}

void zx_ble_spam_scene_menu_on_enter(void* context) {
    App* app = context;
    submenu_reset(app->submenu);

    submenu_add_item(app->submenu, "BTLE Scanner", 0, menu_callback, app);
    submenu_add_item(app->submenu, "FindMy Scanner", 1, menu_callback, app);
    submenu_add_item(app->submenu, "Peripheral Spam", 2, menu_callback, app);
    submenu_add_item(app->submenu, "Sensor Reader", 3, menu_callback, app);
    submenu_add_item(app->submenu, "Device Clone", 4, menu_callback, app);
    submenu_add_item(app->submenu, "MouseJack", 5, menu_callback, app);
    submenu_add_item(app->submenu, "Keyboard Spam", 6, menu_callback, app);
    submenu_add_item(app->submenu, "Remote Trigger", 7, menu_callback, app);
    submenu_add_item(app->submenu, "Sniffer Export", 8, menu_callback, app);
    submenu_add_item(app->submenu, "Channel Scan", 9, menu_callback, app);
    submenu_add_item(app->submenu, "BLE Spam", 10, menu_callback, app);

    if(!app->nrf24_connected) {
        widget_reset(app->widget);
        widget_add_string_element(app->widget, 64, 20, AlignCenter, AlignCenter, FontPrimary, "nRF24 not found");
        widget_add_string_element(app->widget, 64, 40, AlignCenter, AlignCenter, FontSecondary, "Connect module to GPIO");
        widget_add_string_element(app->widget, 64, 55, AlignCenter, AlignCenter, FontSecondary, "PB2(CE) PA4(CS) PB3(CK)");
        widget_add_string_element(app->widget, 64, 65, AlignCenter, AlignCenter, FontSecondary, "PA7(MO) PA6(MI) GND 3V3");
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewWidget);
    } else {
        view_dispatcher_switch_to_view(app->view_dispatcher, AppViewSubmenu);
    }
}

bool zx_ble_spam_scene_menu_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void zx_ble_spam_scene_menu_on_exit(void* context) {
    App* app = context;
    submenu_reset(app->submenu);
    widget_reset(app->widget);
}
