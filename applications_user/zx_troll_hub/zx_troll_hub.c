#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <loader/loader.h>
#include <storage/storage.h>

#define TAG "ZxTrollHub"
#define APPS_PATH "/ext/apps/Troll"

typedef enum {
    TrollAppIndexTvBGone = 0,
    TrollAppIndexCount,
} TrollAppIndex;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
} TrollHub;

static void troll_hub_launch_app(const char* path) {
    Loader* loader = furi_record_open(RECORD_LOADER);
    loader_start_detached_with_gui_error(loader, path, NULL);
    furi_record_close(RECORD_LOADER);
}

static void troll_hub_menu_callback(void* context, uint32_t index) {
    UNUSED(context);
    switch(index) {
    case TrollAppIndexTvBGone:
        troll_hub_launch_app(APPS_PATH "/zx_tv_b_gone.fap");
        break;
    default:
        break;
    }
}

static bool troll_hub_is_app_installed(const char* path) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    bool exists = storage_file_exists(storage, path);
    furi_record_close(RECORD_STORAGE);
    return exists;
}

static uint32_t troll_hub_exit_callback(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static TrollHub* troll_hub_alloc(void) {
    TrollHub* app = malloc(sizeof(TrollHub));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->submenu = submenu_alloc();
    submenu_set_header(app->submenu, "Troll Apps");

    bool tv_b_gone_installed = troll_hub_is_app_installed(APPS_PATH "/zx_tv_b_gone.fap");
    submenu_add_item(
        app->submenu,
        tv_b_gone_installed ? "ZX TV B Gone" : "ZX TV B Gone (non installe)",
        TrollAppIndexTvBGone,
        troll_hub_menu_callback,
        app);

    submenu_add_lockable_item(
        app->submenu,
        "Autres apps... (bientot)",
        99,
        NULL,
        app,
        true,
        "En developpement!");

    view_set_previous_callback(submenu_get_view(app->submenu), troll_hub_exit_callback);
    view_dispatcher_add_view(app->view_dispatcher, 0, submenu_get_view(app->submenu));
    view_dispatcher_switch_to_view(app->view_dispatcher, 0);

    return app;
}

static void troll_hub_free(TrollHub* app) {
    view_dispatcher_remove_view(app->view_dispatcher, 0);
    submenu_free(app->submenu);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);
}

int32_t zx_troll_hub_app(void* p) {
    UNUSED(p);

    TrollHub* app = troll_hub_alloc();
    view_dispatcher_run(app->view_dispatcher);
    troll_hub_free(app);

    return 0;
}
