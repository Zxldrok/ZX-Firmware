#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/elements.h>

#define TAG "ZxSignalScope"

#define SAMPLE_COUNT 128
#define SAMPLE_INTERVAL_MS 50
#define DB_SCALE_MIN -100.0f
#define DB_SCALE_MAX 0.0f

typedef enum {
    Band300MHz = 0,
    Band400MHz,
    Band868MHz,
    Band915MHz,
    BandCount
} FrequencyBand;

static const uint32_t band_frequencies[BandCount] = {
    315000000,
    433920000,
    868350000,
    915000000,
};

static const char* band_names[BandCount] = {
    "315 MHz",
    "433 MHz",
    "868 MHz",
    "915 MHz",
};

typedef struct {
    uint8_t buffer[SAMPLE_COUNT];
    uint8_t peaks[SAMPLE_COUNT];
    float current_rssi;
    uint32_t frequency;
    FrequencyBand band;
    bool frozen;
    bool running;
    uint8_t write_pos;
    FuriTimer* timer;
    Gui* gui;
    ViewPort* view_port;
    float threshold;
} SignalScope;

static float get_rssi_dbm(void) {
    return furi_hal_subghz_get_rssi();
}

static void radio_set_freq(uint32_t freq) {
    furi_hal_subghz_idle();
    furi_hal_subghz_set_frequency_and_path(freq);
    furi_hal_subghz_rx();
    furi_delay_ms(3);
}

static void radio_stop(void) {
    furi_hal_subghz_idle();
    furi_hal_subghz_sleep();
}

static uint8_t db_to_intensity(float dbm) {
    if(dbm < DB_SCALE_MIN) return 0;
    if(dbm > DB_SCALE_MAX) return 63;
    return (uint8_t)((dbm - DB_SCALE_MIN) * 63.0f / (DB_SCALE_MAX - DB_SCALE_MIN));
}

static void scope_sample(SignalScope* scope) {
    if(!scope->running || scope->frozen) return;

    scope->current_rssi = get_rssi_dbm();
    uint8_t val = db_to_intensity(scope->current_rssi);
    scope->buffer[scope->write_pos] = val;
    if(val > scope->peaks[scope->write_pos]) {
        scope->peaks[scope->write_pos] = val;
    }
    scope->write_pos = (scope->write_pos + 1) % SAMPLE_COUNT;
}

static void scope_timer_callback(void* context) {
    SignalScope* scope = context;
    scope_sample(scope);
    view_port_update(scope->view_port);
}

static void draw_waveform(Canvas* canvas, SignalScope* scope) {
    const uint8_t graph_x = 0;
    const uint8_t graph_y = 10;
    const uint8_t graph_w = 128;
    const uint8_t graph_h = 44;

    // Background
    canvas_draw_frame(canvas, graph_x, graph_y - 1, graph_w, graph_h + 2);

    // Grid lines
    for(uint8_t y = 0; y < 4; y++) {
        uint8_t ly = graph_y + (graph_h * (y + 1) / 5);
        canvas_draw_dot(canvas, graph_x + 1, ly);
        canvas_draw_dot(canvas, graph_x + graph_w - 2, ly);
    }

    // Threshold line
    uint8_t thresh_y = graph_y + graph_h - 1 -
                        (uint8_t)((scope->threshold - DB_SCALE_MIN) * graph_h /
                                  (DB_SCALE_MAX - DB_SCALE_MIN));
    if(thresh_y < graph_y) thresh_y = graph_y;
    if(thresh_y > graph_y + graph_h - 1) thresh_y = graph_y + graph_h - 1;
    for(uint8_t x = graph_x + 2; x < graph_x + graph_w - 2; x += 4) {
        canvas_draw_dot(canvas, x, thresh_y);
    }

    // Waveform
    uint32_t start = scope->write_pos;
    for(uint8_t i = 1; i < SAMPLE_COUNT - 1; i++) {
        uint8_t idx = (start + i) % SAMPLE_COUNT;
        uint8_t prev_idx = (start + i - 1) % SAMPLE_COUNT;

        uint8_t y1 = graph_y + graph_h - 1 -
                     (uint8_t)((float)scope->buffer[prev_idx] * graph_h / 63.0f);
        uint8_t y2 = graph_y + graph_h - 1 -
                     (uint8_t)((float)scope->buffer[idx] * graph_h / 63.0f);

        if(y1 > graph_y + graph_h - 1) y1 = graph_y + graph_h - 1;
        if(y2 > graph_y + graph_h - 1) y2 = graph_y + graph_h - 1;

        uint8_t x1 = graph_x + 2 + ((i - 1) * (graph_w - 4) / (SAMPLE_COUNT - 2));
        uint8_t x2 = graph_x + 2 + (i * (graph_w - 4) / (SAMPLE_COUNT - 2));

        canvas_draw_line(canvas, x1, y1, x2, y2);

        // Peak hold
        uint8_t py = graph_y + graph_h - 1 -
                     (uint8_t)((float)scope->peaks[prev_idx] * graph_h / 63.0f);
        if(py > graph_y && py < graph_y + graph_h - 1) {
            canvas_draw_dot(canvas, x1, py);
        }
    }

    // Status bar
    canvas_set_font(canvas, FontSecondary);

    // Left: frequency
    char freq_str[16];
    snprintf(freq_str, sizeof(freq_str), "%u.%02u MHz",
             (unsigned)(scope->frequency / 1000000),
             (unsigned)((scope->frequency % 1000000) / 10000));
    canvas_draw_str(canvas, 2, 8, freq_str);

    // Right: RSSI value
    char rssi_str[12];
    snprintf(rssi_str, sizeof(rssi_str), "%.1f dBm", (double)scope->current_rssi);
    canvas_draw_str(canvas, 78, 8, rssi_str);

    // Bottom info
    char info_str[32];
    if(scope->frozen) {
        snprintf(info_str, sizeof(info_str), "FROZEN  OK:unfreeze");
    } else {
        snprintf(info_str, sizeof(info_str), "Band: %s  Th:%.0f", band_names[scope->band],
                 (double)scope->threshold);
    }
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str(canvas, 2, 62, info_str);
}

static void scope_input_callback(InputEvent* event, void* context) {
    SignalScope* scope = context;
    if(event->type != InputTypeShort && event->type != InputTypeLong) return;

    switch(event->key) {
    case InputKeyOk:
        scope->frozen = !scope->frozen;
        if(!scope->frozen) {
            memset(scope->peaks, 0, sizeof(scope->peaks));
        }
        break;

    case InputKeyLeft:
        if(scope->band > 0) {
            scope->band--;
            scope->frequency = band_frequencies[scope->band];
            radio_set_freq(scope->frequency);
            memset(scope->buffer, 0, sizeof(scope->buffer));
            memset(scope->peaks, 0, sizeof(scope->peaks));
            scope->write_pos = 0;
        }
        break;

    case InputKeyRight:
        if(scope->band < BandCount - 1) {
            scope->band++;
            scope->frequency = band_frequencies[scope->band];
            radio_set_freq(scope->frequency);
            memset(scope->buffer, 0, sizeof(scope->buffer));
            memset(scope->peaks, 0, sizeof(scope->peaks));
            scope->write_pos = 0;
        }
        break;

    case InputKeyUp:
        scope->threshold += 5.0f;
        if(scope->threshold > DB_SCALE_MAX) scope->threshold = DB_SCALE_MAX;
        break;

    case InputKeyDown:
        scope->threshold -= 5.0f;
        if(scope->threshold < DB_SCALE_MIN) scope->threshold = DB_SCALE_MIN;
        break;

    case InputKeyBack:
        if(event->type == InputTypeLong) {
            scope->running = false;
        }
        break;

    default:
        break;
    }
}

static void scope_draw_callback(Canvas* canvas, void* context) {
    SignalScope* scope = context;
    canvas_clear(canvas);
    draw_waveform(canvas, scope);
}

static void scope_alloc(SignalScope* scope) {
    scope->band = Band400MHz;
    scope->frequency = band_frequencies[scope->band];
    scope->threshold = -85.0f;
    scope->frozen = false;
    scope->running = true;
    scope->write_pos = 0;
    memset(scope->buffer, 0, sizeof(scope->buffer));
    memset(scope->peaks, 0, sizeof(scope->peaks));

    // Start radio
    radio_set_freq(scope->frequency);

    // GUI
    scope->gui = furi_record_open(RECORD_GUI);
    scope->view_port = view_port_alloc();
    view_port_draw_callback_set(scope->view_port, scope_draw_callback, scope);
    view_port_input_callback_set(scope->view_port, scope_input_callback, scope);
    gui_add_view_port(scope->gui, scope->view_port, GuiLayerFullscreen);

    // Timer for sampling
    scope->timer =
        furi_timer_alloc(scope_timer_callback, FuriTimerTypePeriodic, scope);
    furi_timer_start(scope->timer, furi_ms_to_ticks(SAMPLE_INTERVAL_MS));
}

static void scope_free(SignalScope* scope) {
    furi_timer_stop(scope->timer);
    furi_timer_free(scope->timer);

    radio_stop();

    gui_remove_view_port(scope->gui, scope->view_port);
    view_port_free(scope->view_port);
    furi_record_close(RECORD_GUI);
}

int32_t zx_signal_scope_app_entry(void* p) {
    UNUSED(p);

    SignalScope* scope = malloc(sizeof(SignalScope));
    scope_alloc(scope);

    while(scope->running) {
        furi_delay_ms(100);
    }

    scope_free(scope);
    free(scope);

    return 0;
}
