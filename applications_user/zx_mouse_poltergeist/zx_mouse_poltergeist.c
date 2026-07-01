#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>

#define TAG "ZxMousePoltergeist"

typedef struct {
    bool running;
    bool active;
    uint32_t move_count;
    Gui* gui;
    ViewPort* view_port;
    FuriTimer* move_timer;
    FuriSemaphore* exit_sem;
} MousePoltergeist;

static void poltergeist_callback(void* context) {
    MousePoltergeist* app = context;
    if(!app->active) return;

    if(!furi_hal_hid_is_connected()) {
        view_port_update(app->view_port);
        return;
    }

    int8_t dx = (rand() % 41) - 20;
    int8_t dy = (rand() % 41) - 20;
    furi_hal_hid_mouse_move(dx, dy);

    if(rand() % 5 == 0) {
        furi_hal_hid_mouse_press(HID_MOUSE_BTN_LEFT);
        furi_delay_ms(50 + rand() % 100);
        furi_hal_hid_mouse_release(HID_MOUSE_BTN_LEFT);
    }

    if(rand() % 10 == 0) {
        furi_hal_hid_mouse_scroll((rand() % 3) - 1);
    }

    app->move_count++;
    view_port_update(app->view_port);
}

static void draw_callback(Canvas* canvas, void* context) {
    MousePoltergeist* app = context;
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "Mouse Poltergeist");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 24, app->active ? "ACTIF" : "INACTIF");

    char buf[32];
    snprintf(buf, sizeof(buf), "Mouvements: %lu", (unsigned long)app->move_count);
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
    MousePoltergeist* app = context;
    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        app->active = !app->active;
        view_port_update(app->view_port);
    }
    if(event->type == InputTypeLong && event->key == InputKeyBack) {
        app->running = false;
        app->active = false;
        furi_timer_stop(app->move_timer);
        furi_semaphore_release(app->exit_sem);
    }
}

static bool app_alloc(MousePoltergeist* app) {
    app->running = true;
    app->active = false;
    app->move_count = 0;

    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    app->exit_sem = furi_semaphore_alloc(1, 0);
    if(!app->exit_sem) return false;

    srand(furi_get_tick());
    app->move_timer = furi_timer_alloc(poltergeist_callback, FuriTimerTypePeriodic, app);
    if(!app->move_timer) {
        furi_semaphore_free(app->exit_sem);
        return false;
    }
    furi_timer_start(app->move_timer, furi_ms_to_ticks(200));

    return true;
}

static void app_free(MousePoltergeist* app) {
    furi_timer_stop(app->move_timer);
    furi_timer_free(app->move_timer);
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    furi_semaphore_free(app->exit_sem);
}

int32_t zx_mouse_poltergeist_app(void* p) {
    UNUSED(p);
    MousePoltergeist* app = malloc(sizeof(MousePoltergeist));
    if(!app_alloc(app)) {
        free(app);
        return 1;
    }
    furi_semaphore_acquire(app->exit_sem, FuriWaitForever);
    app_free(app);
    free(app);
    return 0;
}
