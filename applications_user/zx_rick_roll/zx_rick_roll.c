#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/elements.h>

#define TAG "ZxRickRoll"

#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_REST 0

typedef struct {
    uint16_t freq;
    uint16_t dur;
} Note;

// Simplified Never Gonna Give You Up intro
static const Note melody[] = {
    {NOTE_G4, 150}, {NOTE_A4, 150}, {NOTE_B4, 150}, {NOTE_C5, 150},
    {NOTE_D5, 150}, {NOTE_E5, 150}, {NOTE_F5, 150}, {NOTE_G5, 150},
    {NOTE_G5, 200}, {NOTE_G5, 100}, {NOTE_E5, 200}, {NOTE_E5, 100},
    {NOTE_D5, 200}, {NOTE_C5, 200}, {NOTE_C5, 100}, {NOTE_D5, 100},
    {NOTE_E5, 300}, {NOTE_C5, 100}, {NOTE_D5, 100}, {NOTE_E5, 200},
    {NOTE_F5, 200}, {NOTE_E5, 100}, {NOTE_D5, 100}, {NOTE_C5, 400},
    {NOTE_D5, 200}, {NOTE_E5, 200}, {NOTE_C5, 200}, {NOTE_D5, 200},
    {NOTE_E5, 100}, {NOTE_F5, 100}, {NOTE_E5, 200}, {NOTE_D5, 200},
    {NOTE_C5, 400},
};
#define MELODY_LEN (sizeof(melody) / sizeof(melody[0]))

typedef struct {
    bool running;
    bool playing;
    uint16_t note_index;
    uint8_t frame;
    uint32_t total_played;
    Gui* gui;
    ViewPort* view_port;
    FuriTimer* note_timer;
    FuriTimer* anim_timer;
    FuriSemaphore* exit_sem;
} RickRoll;

static void play_note(RickRoll* app) {
    if(app->note_index >= MELODY_LEN) {
        app->note_index = 0;
        if(!app->playing) return;
    }

    const Note* n = &melody[app->note_index];

    if(n->freq == NOTE_REST) {
        furi_hal_speaker_stop();
    } else {
        if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(30)) {
            furi_hal_speaker_start((float)n->freq, 0.5f);
        }
    }

    furi_timer_start(app->note_timer, furi_ms_to_ticks(n->dur));
    app->note_index++;
    app->total_played++;
}

static void note_callback(void* context) {
    RickRoll* app = context;
    if(!app->playing) return;
    play_note(app);
    view_port_update(app->view_port);
}

static void anim_callback(void* context) {
    RickRoll* app = context;
    app->frame = (app->frame + 1) % 4;
    view_port_update(app->view_port);
}

static void draw_callback(Canvas* canvas, void* context) {
    RickRoll* app = context;
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "Never Gonna...");

    // Simple pixel art Rick - 4 frames of a dancing figure
    uint8_t f = app->frame;
    uint8_t x = 50, y = 30;

    // Head
    canvas_draw_box(canvas, x, y, 8, 6);
    // Eyes
    if(f % 2 == 0) {
        canvas_draw_dot(canvas, x + 2, y + 2);
        canvas_draw_dot(canvas, x + 5, y + 2);
    } else {
        canvas_draw_box(canvas, x + 2, y + 1, 1, 2);
        canvas_draw_box(canvas, x + 5, y + 1, 1, 2);
    }
    // Mouth
    canvas_draw_line(canvas, x + 2, y + 5, x + 5, y + 5);
    // Body
    canvas_draw_box(canvas, x + 2, y + 6, 4, 6);
    // Arms
    int8_t arm_off = (f < 2) ? -2 : 2;
    canvas_draw_line(canvas, x + arm_off, y + 8, x + 8 + arm_off, y + 8);
    // Legs
    canvas_draw_line(canvas, x + 2, y + 12, x + arm_off, y + 15);
    canvas_draw_line(canvas, x + 5, y + 12, x + 8 - arm_off, y + 15);

    canvas_set_font(canvas, FontSecondary);
    char buf[32];
    snprintf(buf, sizeof(buf), "Notes: %lu", (unsigned long)app->total_played);
    canvas_draw_str(canvas, 2, 62, app->playing ? buf : "[OK]: jouer  [Ret] long: quitter");
}

static void input_callback(InputEvent* event, void* context) {
    RickRoll* app = context;
    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        app->playing = !app->playing;
        if(app->playing) {
            app->note_index = 0;
            play_note(app);
            furi_timer_start(app->anim_timer, furi_ms_to_ticks(250));
        } else {
            furi_timer_stop(app->note_timer);
            furi_timer_stop(app->anim_timer);
            furi_hal_speaker_stop();
        }
        view_port_update(app->view_port);
    }
    if(event->type == InputTypeLong && event->key == InputKeyBack) {
        app->running = false;
        app->playing = false;
        furi_timer_stop(app->note_timer);
        furi_timer_stop(app->anim_timer);
        furi_hal_speaker_stop();
        furi_semaphore_release(app->exit_sem);
    }
}

static bool app_alloc(RickRoll* app) {
    app->running = true;
    app->playing = false;
    app->note_index = 0;
    app->frame = 0;
    app->total_played = 0;

    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    app->exit_sem = furi_semaphore_alloc(1, 0);
    if(!app->exit_sem) return false;

    app->note_timer = furi_timer_alloc(note_callback, FuriTimerTypeOnce, app);
    app->anim_timer = furi_timer_alloc(anim_callback, FuriTimerTypePeriodic, app);
    if(!app->note_timer || !app->anim_timer) {
        if(app->note_timer) furi_timer_free(app->note_timer);
        if(app->anim_timer) furi_timer_free(app->anim_timer);
        furi_semaphore_free(app->exit_sem);
        return false;
    }

    return true;
}

static void app_free(RickRoll* app) {
    furi_timer_stop(app->note_timer);
    furi_timer_stop(app->anim_timer);
    furi_timer_free(app->note_timer);
    furi_timer_free(app->anim_timer);
    furi_hal_speaker_stop();
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    furi_semaphore_free(app->exit_sem);
}

int32_t zx_rick_roll_app(void* p) {
    UNUSED(p);
    RickRoll* app = malloc(sizeof(RickRoll));
    if(!app_alloc(app)) {
        free(app);
        return 1;
    }
    furi_semaphore_acquire(app->exit_sem, FuriWaitForever);
    app_free(app);
    free(app);
    return 0;
}
