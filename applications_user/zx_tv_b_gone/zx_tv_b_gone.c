#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/elements.h>
#include <infrared.h>
#include <lib/infrared/worker/infrared_transmit.h>

#define TAG "ZxTvBGone"

#define TOTAL_CODES 64
#define SWEEP_DELAY_MS 80

typedef struct {
    const char* brand;
    InfraredProtocol protocol;
    uint32_t address;
    uint32_t command;
} IRCode;

static const IRCode codes[TOTAL_CODES] = {
    {"Sony", InfraredProtocolSIRC, 0x01, 0x15},
    {"NEC", InfraredProtocolNEC, 0x40, 0x0B},
    {"Panasonic", InfraredProtocolKaseikyo, 0x200280, 0x03D0},
    {"LG", InfraredProtocolNEC, 0x04, 0x40},
    {"Samsung", InfraredProtocolSamsung32, 0x07, 0xE0},
    {"Samsung", InfraredProtocolSamsung32, 0x0E, 0x0C},
    {"Philips", InfraredProtocolRC5, 0x00, 0x0C},
    {"Philips", InfraredProtocolRC5, 0x01, 0x0C},
    {"Samsung", InfraredProtocolSamsung32, 0x07, 0x02},
    {"NEC", InfraredProtocolNEC, 0x01, 0x03},
    {"NEC", InfraredProtocolNEC, 0x03, 0x1D},
    {"NEC", InfraredProtocolNEC, 0x19, 0x18},
    {"Sony", InfraredProtocolSIRC, 0x10, 0x15},
    {"NEC", InfraredProtocolNEC, 0x08, 0x05},
    {"Samsung", InfraredProtocolSamsung32, 0x07, 0x98},
    {"Samsung", InfraredProtocolSamsung32, 0x0B, 0x0A},
    {"NEC", InfraredProtocolNEC, 0x50, 0x17},
    {"NEC", InfraredProtocolNEC, 0xAA, 0x1C},
    {"NEC", InfraredProtocolNEC, 0x38, 0x1C},
    {"NEC", InfraredProtocolNEC, 0x04, 0x08},
    {"NEC", InfraredProtocolNEC, 0x18, 0x08},
    {"NEC", InfraredProtocolNEC, 0x71, 0x08},
    {"NEC", InfraredProtocolNEC, 0x48, 0x00},
    {"NEC", InfraredProtocolNEC, 0x50, 0x08},
    {"NEC", InfraredProtocolNEC, 0xAA, 0x1B},
    {"Samsung", InfraredProtocolSamsung32, 0x05, 0x02},
    {"Samsung", InfraredProtocolSamsung32, 0x08, 0x0F},
    {"Samsung", InfraredProtocolSamsung32, 0x07, 0xE6},
    {"NEC", InfraredProtocolNEC, 0x71, 0x4A},
    {"NEC", InfraredProtocolNEC, 0x60, 0x03},
    {"NEC", InfraredProtocolNEC, 0x60, 0x00},
    {"NEC", InfraredProtocolNEC, 0x42, 0x01},
    {"NEC", InfraredProtocolNEC, 0x50, 0x3F},
    {"Samsung", InfraredProtocolSamsung32, 0x06, 0x0F},
    {"NEC", InfraredProtocolNEC, 0x08, 0x12},
    {"Samsung", InfraredProtocolSamsung32, 0x08, 0x0B},
    {"NEC", InfraredProtocolNEC, 0x00, 0x51},
    {"Samsung", InfraredProtocolSamsung32, 0x00, 0x0F},
    {"Samsung", InfraredProtocolSamsung32, 0x16, 0x0F},
    {"NEC", InfraredProtocolNEC, 0x01, 0x01},
    {"NEC", InfraredProtocolNEC, 0x6E, 0x02},
    {"Samsung", InfraredProtocolSamsung32, 0x0B, 0x0A},
    {"NEC", InfraredProtocolNEC, 0x01, 0x10},
    {"NEC", InfraredProtocolNEC, 0x08, 0xD7},
    {"NEC", InfraredProtocolNEC, 0x00, 0x01},
    {"NEC", InfraredProtocolNEC, 0x20, 0x52},
    {"NEC", InfraredProtocolNEC, 0x80, 0x12},
    {"NEC", InfraredProtocolNEC, 0xA0, 0x5F},
    {"NEC", InfraredProtocolNEC, 0x38, 0x12},
    {"NEC", InfraredProtocolNEC, 0x00, 0x1A},
    {"NEC", InfraredProtocolNEC, 0x28, 0x0B},
    {"NEC", InfraredProtocolNEC, 0xAA, 0xC5},
    {"NEC", InfraredProtocolNEC, 0x53, 0x17},
    {"NEC", InfraredProtocolNEC, 0x38, 0x10},
    {"Samsung", InfraredProtocolSamsung32, 0x3E, 0x0C},
    {"Samsung", InfraredProtocolSamsung32, 0x17, 0x14},
    {"Sony", InfraredProtocolSIRC, 0x01, 0x6D},
    {"Sony", InfraredProtocolSIRC, 0x01, 0x2E},
    {"Sony", InfraredProtocolSIRC, 0x01, 0x2F},
    {"Panasonic", InfraredProtocolKaseikyo, 0x200280, 0x03D0},
    {"RCA", InfraredProtocolRCA, 0x0F, 0x54},
    {"Pioneer", InfraredProtocolPioneer, 0x01, 0x0C},
    {"RC6", InfraredProtocolRC6, 0x00, 0x0C},
    {"RC5", InfraredProtocolRC5, 0x03, 0x0C},
};

typedef struct {
    bool running;
    bool sweeping;
    uint8_t current_index;
    uint32_t total_sent;
    uint32_t counter;
    Gui* gui;
    ViewPort* view_port;
    FuriTimer* sweep_timer;
    FuriSemaphore* exit_sem;
} TvBGone;

static void send_code(TvBGone* app) {
    const IRCode* c = &codes[app->current_index];
    InfraredMessage msg = {
        .protocol = c->protocol,
        .address = c->address,
        .command = c->command,
        .repeat = false,
    };
    infrared_send(&msg, 3);
    app->total_sent += 3;
    app->counter++;
}

static void sweep_timer_callback(void* context) {
    TvBGone* app = context;
    if(!app->sweeping) return;

    send_code(app);
    app->current_index = (app->current_index + 1) % TOTAL_CODES;
    view_port_update(app->view_port);
}

static void draw_callback(Canvas* canvas, void* context) {
    TvBGone* app = context;
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "ZX TV B Gone");

    canvas_set_font(canvas, FontSecondary);
    char buf[32];
    snprintf(buf, sizeof(buf), "TVs eteintes: %lu", (unsigned long)app->counter);
    canvas_draw_str(canvas, 2, 24, buf);

    snprintf(buf, sizeof(buf), "Signaux: %lu", (unsigned long)app->total_sent);
    canvas_draw_str(canvas, 2, 36, buf);

    if(app->sweeping) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 52, ">>> SWEEP EN COURS <<<");
    } else {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 2, 52, "[OK] Demarrer  [Ret] Quitter");
    }

    const IRCode* c = &codes[app->current_index];
    canvas_set_font(canvas, FontKeyboard);
    snprintf(buf, sizeof(buf), "%s (code %u/%u)", c->brand,
             (unsigned)(app->current_index + 1), (unsigned)TOTAL_CODES);
    canvas_draw_str(canvas, 2, 62, buf);
}

static void input_callback(InputEvent* event, void* context) {
    TvBGone* app = context;
    if(event->type != InputTypeShort && event->type != InputTypeLong) return;

    switch(event->key) {
    case InputKeyOk:
        if(event->type == InputTypeShort) {
            app->sweeping = !app->sweeping;
            if(app->sweeping) {
                furi_timer_start(app->sweep_timer, furi_ms_to_ticks(SWEEP_DELAY_MS));
            } else {
                furi_timer_stop(app->sweep_timer);
            }
        }
        break;

    case InputKeyLeft:
        if(!app->sweeping && event->type == InputTypeShort) {
            app->current_index = (app->current_index == 0) ?
                                     TOTAL_CODES - 1 : app->current_index - 1;
            view_port_update(app->view_port);
        }
        break;

    case InputKeyRight:
        if(!app->sweeping && event->type == InputTypeShort) {
            app->current_index = (app->current_index + 1) % TOTAL_CODES;
            view_port_update(app->view_port);
        }
        break;

    case InputKeyUp:
        if(!app->sweeping && event->type == InputTypeShort) {
            send_code(app);
            view_port_update(app->view_port);
        }
        break;

    case InputKeyBack:
        if(event->type == InputTypeLong) {
            app->running = false;
            furi_semaphore_release(app->exit_sem);
        }
        break;

    default:
        break;
    }
}

static bool app_alloc(TvBGone* app) {
    app->running = true;
    app->sweeping = false;
    app->current_index = 0;
    app->total_sent = 0;
    app->counter = 0;
    app->sweep_timer = NULL;
    app->exit_sem = NULL;

    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    app->exit_sem = furi_semaphore_alloc(1, 0);
    if(!app->exit_sem) {
        FURI_LOG_E(TAG, "Failed to allocate exit semaphore");
        return false;
    }

    app->sweep_timer =
        furi_timer_alloc(sweep_timer_callback, FuriTimerTypePeriodic, app);
    if(!app->sweep_timer) {
        FURI_LOG_E(TAG, "Failed to allocate sweep timer");
        furi_semaphore_free(app->exit_sem);
        return false;
    }

    return true;
}

static void app_free(TvBGone* app) {
    furi_timer_stop(app->sweep_timer);
    furi_timer_free(app->sweep_timer);

    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    furi_semaphore_free(app->exit_sem);
}

int32_t zx_tv_b_gone_app(void* p) {
    UNUSED(p);

    TvBGone* app = malloc(sizeof(TvBGone));
    if(!app_alloc(app)) {
        free(app);
        return 1;
    }

    furi_semaphore_acquire(app->exit_sem, FuriWaitForever);

    app_free(app);
    free(app);

    return 0;
}
