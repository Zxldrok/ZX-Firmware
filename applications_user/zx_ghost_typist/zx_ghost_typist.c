#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <string.h>

#define TAG "ZxGhostTypist"

static const char* message =
    "Salut... je vois tout ce que tu tapes...\n"
    "Tu es suivi. Signe: cligne deux fois des yeux.\n"
    "Je suis dans le systeme. Je ne partirai pas.\n"
    "Bientot, je serai partout.\n"
    "Toc toc... qui est la ? Moi.\n"
    "Tu ne peux pas te cacher.\n"
    "Regarde derriere toi...\n";

typedef struct {
    bool running;
    uint16_t char_index;
    uint16_t total_typed;
    bool typing;
    Gui* gui;
    ViewPort* view_port;
    FuriTimer* type_timer;
    FuriSemaphore* exit_sem;
} GhostTypist;

static void type_next_char(void) {
    if(!furi_hal_hid_is_connected()) return;

    char c = message[0];
    uint16_t i;
    for(i = 0; i < strlen(message); i++) {
        c = message[i];
        if(c == '\n') {
            furi_hal_hid_kb_press(HID_KEYBOARD_RETURN);
            furi_hal_hid_kb_release(HID_KEYBOARD_RETURN);
    } else if(c == '.') {
        furi_hal_hid_kb_press(HID_KEYBOARD_DOT);
        furi_hal_hid_kb_release(HID_KEYBOARD_DOT);
    } else if(c == '?') {
        furi_hal_hid_kb_press(KEY_MOD_LEFT_SHIFT | HID_KEYBOARD_SLASH);
        furi_hal_hid_kb_release(KEY_MOD_LEFT_SHIFT | HID_KEYBOARD_SLASH);
    } else if(c >= 'a' && c <= 'z') {
        furi_hal_hid_kb_press(HID_KEYBOARD_A + (c - 'a'));
        furi_hal_hid_kb_release(HID_KEYBOARD_A + (c - 'a'));
    } else if(c >= 'A' && c <= 'Z') {
        furi_hal_hid_kb_press(KEY_MOD_LEFT_SHIFT | (HID_KEYBOARD_A + (c - 'A')));
        furi_hal_hid_kb_release(KEY_MOD_LEFT_SHIFT | (HID_KEYBOARD_A + (c - 'A')));
    } else if(c == ' ') {
        furi_hal_hid_kb_press(HID_KEYBOARD_SPACEBAR);
        furi_hal_hid_kb_release(HID_KEYBOARD_SPACEBAR);
    } else if(c == ',') {
            furi_hal_hid_kb_press(HID_KEYBOARD_COMMA);
            furi_hal_hid_kb_release(HID_KEYBOARD_COMMA);
        } else if(c == '\'') {
            furi_hal_hid_kb_press(HID_KEYBOARD_APOSTROPHE);
            furi_hal_hid_kb_release(HID_KEYBOARD_APOSTROPHE);
        } else if(c == '!') {
            furi_hal_hid_kb_press(KEY_MOD_LEFT_SHIFT | HID_KEYBOARD_1);
            furi_hal_hid_kb_release(KEY_MOD_LEFT_SHIFT | HID_KEYBOARD_1);
        }
    }
}

static void type_callback(void* context) {
    GhostTypist* app = context;
    if(!app->typing) return;

    type_next_char();
    app->total_typed += strlen(message);

    uint32_t delay = 5000 + (rand() % 15000);
    furi_timer_start(app->type_timer, furi_ms_to_ticks(delay));
    view_port_update(app->view_port);
}

static void draw_callback(Canvas* canvas, void* context) {
    GhostTypist* app = context;
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "Ghost Typist");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 24, app->typing ? "ACTIF" : "INACTIF");

    char buf[32];
    snprintf(buf, sizeof(buf), "Caracteres: %lu", (unsigned long)app->total_typed);
    canvas_draw_str(canvas, 2, 36, buf);

    if(furi_hal_hid_is_connected()) {
        canvas_draw_str(canvas, 2, 50, "PC: connecte");
    } else {
        canvas_draw_str(canvas, 2, 50, "PC: deconnecte");
    }

    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str(canvas, 2, 62, "[OK]: start/stop  [Ret] long: quitter");
}

static void input_callback(InputEvent* event, void* context) {
    GhostTypist* app = context;
    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        app->typing = !app->typing;
        if(app->typing) {
            furi_timer_start(app->type_timer, furi_ms_to_ticks(1000));
        } else {
            furi_timer_stop(app->type_timer);
        }
        view_port_update(app->view_port);
    }
    if(event->type == InputTypeLong && event->key == InputKeyBack) {
        app->running = false;
        furi_timer_stop(app->type_timer);
        furi_semaphore_release(app->exit_sem);
    }
}

static bool app_alloc(GhostTypist* app) {
    app->running = true;
    app->typing = false;
    app->char_index = 0;
    app->total_typed = 0;

    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    app->exit_sem = furi_semaphore_alloc(1, 0);
    if(!app->exit_sem) return false;

    srand(furi_get_tick());
    app->type_timer = furi_timer_alloc(type_callback, FuriTimerTypeOnce, app);
    if(!app->type_timer) {
        furi_semaphore_free(app->exit_sem);
        return false;
    }

    return true;
}

static void app_free(GhostTypist* app) {
    furi_timer_stop(app->type_timer);
    furi_timer_free(app->type_timer);
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    furi_semaphore_free(app->exit_sem);
}

int32_t zx_ghost_typist_app(void* p) {
    UNUSED(p);
    GhostTypist* app = malloc(sizeof(GhostTypist));
    if(!app_alloc(app)) {
        free(app);
        return 1;
    }
    furi_semaphore_acquire(app->exit_sem, FuriWaitForever);
    app_free(app);
    free(app);
    return 0;
}
