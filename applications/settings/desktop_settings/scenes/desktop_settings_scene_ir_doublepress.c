#include "../desktop_settings_app.h"
#include "desktop_settings_scene.h"
#include "desktop_settings_scene_i.h"
#include <storage/storage.h>
#include <dialogs/dialogs.h>
#include <lib/toolbox/path.h>

#define IR_EXTENSION ".ir"

static bool ir_fap_selector_item_callback(
    FuriString* file_path,
    void* context,
    uint8_t** icon_ptr,
    FuriString* item_name) {
    UNUSED(context);
    UNUSED(icon_ptr);
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FuriString* tmp_name = furi_string_alloc();
    path_extract_filename(file_path, tmp_name, true);
    furi_string_set(item_name, tmp_name);
    furi_string_free(tmp_name);
    furi_record_close(RECORD_STORAGE);
    return true;
}

void desktop_settings_scene_ir_doublepress_on_enter(void* context) {
    DesktopSettingsApp* app = context;
    TextInput* text_input = app->text_input;

    text_input_set_header_text(text_input, "IR file path (double-press Left)");

    text_input_set_result_callback(
        text_input,
        NULL,
        app,
        app->settings.ir_doublepress_path,
        sizeof(app->settings.ir_doublepress_path),
        true);

    FuriString* temp_path = furi_string_alloc_set_str(app->settings.ir_doublepress_path[0]
                                                           ? app->settings.ir_doublepress_path
                                                           : EXT_PATH("infrared"));
    const DialogsFileBrowserOptions browser_options = {
        .extension = IR_EXTENSION,
        .skip_assets = true,
        .hide_ext = true,
        .item_loader_callback = ir_fap_selector_item_callback,
        .item_loader_context = app,
        .base_path = EXT_PATH("infrared"),
    };

    if(dialog_file_browser_show(app->dialogs, temp_path, temp_path, &browser_options)) {
        strlcpy(
            app->settings.ir_doublepress_path,
            furi_string_get_cstr(temp_path),
            sizeof(app->settings.ir_doublepress_path));
        desktop_settings_save(&app->settings);
    }

    furi_string_free(temp_path);
    scene_manager_previous_scene(app->scene_manager);
}

bool desktop_settings_scene_ir_doublepress_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void desktop_settings_scene_ir_doublepress_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    text_input_reset(app->text_input);
}
