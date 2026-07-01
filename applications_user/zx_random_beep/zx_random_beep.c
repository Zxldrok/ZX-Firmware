#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>

#define TAG "ZxRandomBeep"

typedef struct {
    bool running;
    uint32_t beep_count;
    Gui* gui;
    ViewPort* view_port;
    FuriTimer* beep_timer;
    FuriSemaphore* exit_sem;
} RandomBeep;

static void beep_callback(void* context) {
    RandomBeep* app = context;
    if(!app->running) return;

    float freq = 200.0f + (float)(rand() % 3000);
    uint32_t dur = 50 + (rand() % 200);

    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(100)) {
        furi_hal_speaker_start(freq, 0.3f);
        furi_delay_ms(dur);
        furi_hal_speaker_stop();
        furi_hal_speaker_release();
    }

    app->beep_count++;
    view_port_update(app->view_port);
}

static void draw_callback(Canvas* canvas, void* context) {
    RandomBeep* app = context;
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "ZX Random Beep");

    canvas_set_font(canvas, FontSecondary);
    char buf[32];
    snprintf(buf, sizeof(buf), "Bip: %lu", (unsigned long)app->beep_count);
    canvas_draw_str(canvas, 2, 26, buf);
    canvas_draw_str(canvas, 2, 40, "Bip toutes les 30-60s");

    canvas_draw_str(canvas, 2, 62, "[Ret] long: quitter");
}

static void input_callback(InputEvent* event, void* context) {
    RandomBeep* app = context;
    if(event->type == InputTypeLong && event->key == InputKeyBack) {
        app->running = false;
        furi_timer_stop(app->beep_timer);
        furi_semaphore_release(app->exit_sem);
    }
}

static bool app_alloc(RandomBeep* app) {
    app->running = true;
    app->beep_count = 0;

    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    app->exit_sem = furi_semaphore_alloc(1, 0);
    if(!app->exit_sem) return false;

    srand(furi_get_tick());
    uint32_t interval = 30000 + (rand() % 30000);
    app->beep_timer = furi_timer_alloc(beep_callback, FuriTimerTypePeriodic, app);
    if(!app->beep_timer) {
        furi_semaphore_free(app->exit_sem);
        return false;
    }
    furi_timer_start(app->beep_timer, furi_ms_to_ticks(interval));

    return true;
}

static void app_free(RandomBeep* app) {
    furi_timer_stop(app->beep_timer);
    furi_timer_free(app->beep_timer);
    furi_hal_speaker_stop();
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    furi_semaphore_free(app->exit_sem);
}

int32_t zx_random_beep_app(void* p) {
    UNUSED(p);
    RandomBeep* app = malloc(sizeof(RandomBeep));
    if(!app_alloc(app)) {
        free(app);
        return 1;
    }
    furi_semaphore_acquire(app->exit_sem, FuriWaitForever);
    app_free(app);
    free(app);
    return 0;
}
