#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/elements.h>
#include <infrared.h>
#include <lib/infrared/worker/infrared_transmit.h>

#define TAG "ZxIrMulti"

#define TOTAL_CODES 60
#define SWEEP_DELAY_MS 80

typedef struct {
    const char* device;
    InfraredProtocol protocol;
    uint32_t address;
    uint32_t command;
} IRCode;

static const IRCode codes[TOTAL_CODES] = {
    // Projectors (16 codes)
    {"Projo NEC", InfraredProtocolNEC, 0x08, 0x0B},
    {"Projo Epson", InfraredProtocolNECext, 0x1308, 0x8778},
    {"Projo BenQ", InfraredProtocolNEC, 0x02, 0x1D},
    {"Projo ViewSonic", InfraredProtocolNECext, 0x83F4, 0x4FB0},
    {"Projo Optoma", InfraredProtocolNECext, 0x8019, 0x10EF},
    {"Projo Sony", InfraredProtocolNEC, 0x80, 0x51},
    {"Projo NEC2", InfraredProtocolNECext, 0x4040, 0x0AF5},
    {"Projo Vivitek", InfraredProtocolNECext, 0x3000, 0x4EB1},
    {"Projo Mitsubishi", InfraredProtocolNECext, 0x1608, 0x8778},
    {"Projo LG", InfraredProtocolNEC, 0x01, 0x01},
    {"Projo Hitachi", InfraredProtocolNECext, 0x84F4, 0x0BF4},
    {"Projo Sharp", InfraredProtocolNECext, 0xDF00, 0x1CE3},
    {"Projo Acer", InfraredProtocolNEC, 0x31, 0x91},
    {"Projo Panasonic", InfraredProtocolNECext, 0x3000, 0x4FB0},
    {"Projo Ask", InfraredProtocolSamsung32, 0x07, 0xE6},
    {"Projo Sanyo", InfraredProtocolNECext, 0x87B8, 0x0CF3},

    // Soundbars / Audio (22 codes)
    {"Son Sony", InfraredProtocolNEC, 0x20, 0x41},
    {"Son Samsung", InfraredProtocolSIRC15, 0x54, 0x15},
    {"Son Bose", InfraredProtocolNECext, 0x8686, 0x0000},
    {"Son JBL", InfraredProtocolNECext, 0x3086, 0x0000},
    {"Son Yamaha", InfraredProtocolNECext, 0x874E, 0x0D00},
    {"Son Philips", InfraredProtocolNEC, 0x03, 0x1D},
    {"Son LG", InfraredProtocolNEC, 0x02, 0x14},
    {"Son Vizio", InfraredProtocolNECext, 0xB857, 0x0CF3},
    {"Son Denon", InfraredProtocolNECext, 0x8745, 0x17E8},
    {"Son Panasonic", InfraredProtocolKaseikyo, 0x415432, 0x0500},
    {"Son Onkyo", InfraredProtocolNECext, 0x4F50, 0x02FD},
    {"Son Marantz", InfraredProtocolNECext, 0x04B1, 0x58A7},
    {"Son Sonos", InfraredProtocolNECext, 0x8BCA, 0x12ED},
    {"Son Pioneer", InfraredProtocolNECext, 0xBD00, 0x01FE},
    {"Son Harman", InfraredProtocolNECext, 0x874E, 0x17E8},
    {"Son Polk", InfraredProtocolNECext, 0x8103, 0xF00F},
    {"Son Klipsch", InfraredProtocolNECext, 0x4850, 0x02FD},
    {"Son Sharp", InfraredProtocolNEC, 0x30, 0x0B},
    {"Son Toshiba", InfraredProtocolNECext, 0x866B, 0x0AF5},
    {"Son JVC", InfraredProtocolNECext, 0x006A, 0x40BF},
    {"Son Aiwa", InfraredProtocolNECext, 0x040F, 0xAD52},
    {"Son Sanyo", InfraredProtocolNECext, 0x8181, 0xF00F},

    // Air conditioners (22 codes)
    {"Clim Mitsubishi", InfraredProtocolNECext, 0xF483, 0x4FB0},
    {"Clim Daikin", InfraredProtocolNECext, 0xF483, 0x17E8},
    {"Clim Fujitsu", InfraredProtocolNECext, 0x4E87, 0x0D00},
    {"Clim Panasonic", InfraredProtocolNECext, 0x3000, 0x4EB1},
    {"Clim Toshiba", InfraredProtocolNECext, 0x87F4, 0x17E8},
    {"Clim Sharp", InfraredProtocolNECext, 0x75F4, 0x0BF4},
    {"Clim Sanyo", InfraredProtocolNECext, 0x19F4, 0x4EB1},
    {"Clim Carrier", InfraredProtocolNECext, 0xE084, 0x20DF},
    {"Clim LG", InfraredProtocolNEC, 0x32, 0x02},
    {"Clim Samsung", InfraredProtocolNEC, 0x32, 0x2E},
    {"Clim Hitachi", InfraredProtocolNECext, 0x00B8, 0x4EB1},
    {"Clim Gree", InfraredProtocolNECext, 0x4E87, 0x17E8},
    {"Clim Midea", InfraredProtocolNECext, 0x4F50, 0x02FD},
    {"Clim Haier", InfraredProtocolNECext, 0x0204, 0x58A7},
    {"Clim Electra", InfraredProtocolNECext, 0x83F4, 0x4EB1},
    {"Clim York", InfraredProtocolNECext, 0x0004, 0xDF20},
    {"Clim Trane", InfraredProtocolNECext, 0x0004, 0xF00F},
    {"Clim Whirlpool", InfraredProtocolNECext, 0x0004, 0x0AF5},
    {"Clim Hisense", InfraredProtocolNECext, 0x0204, 0xAD52},
    {"Clim Fuji", InfraredProtocolNECext, 0x4E87, 0x0CF3},
    {"Clim Rheem", InfraredProtocolNECext, 0x2004, 0x4FB0},
    {"Clim Goodman", InfraredProtocolNECext, 0x2004, 0x17E8},
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
} IrMulti;

static void send_code(IrMulti* app) {
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
    IrMulti* app = context;
    if(!app->sweeping) return;
    send_code(app);
    app->current_index = (app->current_index + 1) % TOTAL_CODES;
    view_port_update(app->view_port);
}

static void draw_callback(Canvas* canvas, void* context) {
    IrMulti* app = context;
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "ZX IR Multi");

    canvas_set_font(canvas, FontSecondary);
    char buf[32];
    snprintf(buf, sizeof(buf), "Appareils eteints: %lu", (unsigned long)app->counter);
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
    snprintf(buf, sizeof(buf), "%s (%u/%u)", c->device,
             (unsigned)(app->current_index + 1), (unsigned)TOTAL_CODES);
    canvas_draw_str(canvas, 2, 62, buf);
}

static void input_callback(InputEvent* event, void* context) {
    IrMulti* app = context;
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
            app->current_index = (app->current_index == 0) ? TOTAL_CODES - 1 : app->current_index - 1;
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

static bool app_alloc(IrMulti* app) {
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
    if(!app->exit_sem) return false;

    app->sweep_timer = furi_timer_alloc(sweep_timer_callback, FuriTimerTypePeriodic, app);
    if(!app->sweep_timer) {
        furi_semaphore_free(app->exit_sem);
        return false;
    }
    return true;
}

static void app_free(IrMulti* app) {
    furi_timer_stop(app->sweep_timer);
    furi_timer_free(app->sweep_timer);
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    furi_semaphore_free(app->exit_sem);
}

int32_t zx_ir_multi_app(void* p) {
    UNUSED(p);
    IrMulti* app = malloc(sizeof(IrMulti));
    if(!app_alloc(app)) {
        free(app);
        return 1;
    }
    furi_semaphore_acquire(app->exit_sem, FuriWaitForever);
    app_free(app);
    free(app);
    return 0;
}
