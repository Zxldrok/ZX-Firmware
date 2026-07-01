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
    TrollAppIndexIrMulti,
    TrollAppIndexRickRoll,
    TrollAppIndexGhostTypist,
    TrollAppIndexMousePoltergeist,
    TrollAppIndexRandomBeep,
    TrollAppIndexCount,
} TrollAppIndex;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
} TrollHub;

typedef struct {
    TrollAppIndex index;
    const char* name;
    const char* fap_path;
} TrollAppEntry;

static const TrollAppEntry apps[] = {
    {TrollAppIndexTvBGone, "ZX TV B Gone", APPS_PATH "/zx_tv_b_gone.fap"},
    {TrollAppIndexIrMulti, "ZX IR Multi (projo/son/clim)", APPS_PATH "/zx_ir_multi.fap"},
    {TrollAppIndexRickRoll, "ZX Rick Roll (musique + anim)", APPS_PATH "/zx_rick_roll.fap"},
    {TrollAppIndexGhostTypist, "ZX Ghost Typist (clavier)", APPS_PATH "/zx_ghost_typist.fap"},
    {TrollAppIndexMousePoltergeist, "ZX Mouse Poltergeist", APPS_PATH "/zx_mouse_poltergeist.fap"},
    {TrollAppIndexRandomBeep, "ZX Random Beep (bip aleatoire)", APPS_PATH "/zx_random_beep.fap"},
};
#define APPS_COUNT (sizeof(apps) / sizeof(apps[0]))

static void troll_hub_launch_app(const char* path) {
    Loader* loader = furi_record_open(RECORD_LOADER);
    loader_start_detached_with_gui_error(loader, path, NULL);
    furi_record_close(RECORD_LOADER);
}

static void troll_hub_menu_callback(void* context, uint32_t index) {
    UNUSED(context);
    for(size_t i = 0; i < APPS_COUNT; i++) {
        if(apps[i].index == index) {
            troll_hub_launch_app(apps[i].fap_path);
            break;
        }
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

    for(size_t i = 0; i < APPS_COUNT; i++) {
        bool installed = troll_hub_is_app_installed(apps[i].fap_path);
        submenu_add_item(
            app->submenu,
            installed ? apps[i].name : "---",
            apps[i].index,
            installed ? troll_hub_menu_callback : NULL,
            app);
    }

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
