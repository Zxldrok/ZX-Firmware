#include "scanner_view.h"
#include "../zx_ble_spam.h"
#include <gui/canvas.h>
#include <stdio.h>
#include <string.h>
#include <furi.h>

typedef struct {
    App* app;
} ScannerViewModel;

static void draw_callback(Canvas* canvas, void* _model) {
    ScannerViewModel* model = _model;
    if(!model || !model->app) return;
    App* app = model->app;

    uint8_t w = canvas_width(canvas);
    uint8_t h = canvas_height(canvas);

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 10, "BTLE Scanner");

    canvas_draw_line(canvas, 0, 14, w - 1, 14);

    canvas_set_font(canvas, FontSecondary);
    char info[32];
    uint8_t ch = (app->scan_channel % 3 == 0) ? 37 : (app->scan_channel % 3 == 1) ? 38 : 39;
    snprintf(info, sizeof(info), "CH%d  %d pkt", ch, app->scan_count);
    canvas_draw_str(canvas, w - 60, 10, info);

    const char spin[] = {'|', '/', '-', '\\'};
    uint8_t idx = (app->anim_frame / 4) % 4;
    canvas_draw_str(canvas, 4, 26, &spin[idx]);

    canvas_draw_str(canvas, 14, 26, "Scanning");

    uint8_t start_y = 34;
    uint8_t max_items = (h - start_y) / 10;
    if(max_items > 10) max_items = 10;

    for(uint8_t i = 0; i < max_items && i < app->scan_count; i++) {
        uint8_t idx2 = app->scan_count - 1 - i;
        ScanResult* r = &app->scan_results[idx2];

        uint8_t bar_len = 0;
        if(r->rssi > -30) bar_len = 28;
        else if(r->rssi > -50) bar_len = 22;
        else if(r->rssi > -70) bar_len = 16;
        else if(r->rssi > -90) bar_len = 10;
        else bar_len = 4;

        uint8_t y = start_y + i * 10;
        canvas_draw_box(canvas, 4, y + 2, bar_len, 5);
        canvas_draw_frame(canvas, 4, y + 2, 28, 5);

        canvas_set_font(canvas, FontSecondary);
        char mac[20];
        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
            r->mac[0], r->mac[1], r->mac[2],
            r->mac[3], r->mac[4], r->mac[5]);
        canvas_draw_str(canvas, 36, y + 8, mac);

        char rssi_str[8];
        snprintf(rssi_str, sizeof(rssi_str), "%ddBm", r->rssi);
        canvas_draw_str(canvas, w - 40, y + 8, rssi_str);
    }

    canvas_draw_line(canvas, 0, h - 11, w - 1, h - 11);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 4, h - 3, app->nrf24_connected ? "nRF24: OK" : "nRF24: ---");
}

static bool input_callback(InputEvent* event, void* context) {
    App* app = context;
    if(!app || !event) return false;

    if(event->type == InputTypePress && event->key == InputKeyOk) {
        view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventToggleScan);
        return true;
    }
    return false;
}

View* scanner_view_alloc(App* app) {
    View* view = view_alloc();
    view_allocate_model(view, ViewModelTypeLocking, sizeof(ScannerViewModel));
    view_set_context(view, app);
    view_set_draw_callback(view, draw_callback);
    view_set_input_callback(view, input_callback);

    with_view_model(view, ScannerViewModel * model, {
        model->app = app;
    }, true);

    return view;
}

void scanner_view_update(App* app) {
    app->anim_frame++;
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventUpdateDisplay);
}
