#include "ble_spam_view.h"
#include "../zx_ble_spam.h"
#include <gui/canvas.h>
#include <stdio.h>
#include <string.h>
#include <furi.h>

static const char* device_names[] = {
    "iPhone 15",
    "AirPods Pro",
    "AirTag",
    "Samsung S24",
    "SmartTag2",
    "Tile Pro",
    "Pixel 9",
    "iBeacon",
    "Surface Pro",
    "LG TV",
};

static void draw_callback(Canvas* canvas, void* context) {
    App* app = context;
    if(!app) return;

    uint8_t w = canvas_width(canvas);
    uint8_t h = canvas_height(canvas);

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 10, "BLE Spam");

    canvas_draw_line(canvas, 0, 14, w - 1, 14);

    if(app->ble_spam_active) {
        canvas_draw_str(canvas, w - 52, 10, "[ACTIVE]");

        canvas_set_font(canvas, FontSecondary);
        char buf[32];
        snprintf(buf, sizeof(buf), "Pkts: %lu", app->ble_spam_packets);
        canvas_draw_str(canvas, 4, 28, buf);

        uint8_t idx = app->ble_spam_type % 10;
        snprintf(buf, sizeof(buf), "Dev: %s", device_names[idx]);
        canvas_draw_str(canvas, 4, 40, buf);

        canvas_set_font(canvas, FontKeyboard);
        snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
            app->ble_spam_mac[0], app->ble_spam_mac[1],
            app->ble_spam_mac[2], app->ble_spam_mac[3],
            app->ble_spam_mac[4], app->ble_spam_mac[5]);
        canvas_draw_str(canvas, 4, 52, buf);

        canvas_set_font(canvas, FontSecondary);
        const char spin[] = {'|', '/', '-', '\\'};
        canvas_draw_str(canvas, 4, h - 4, &spin[(app->anim_frame / 3) % 4]);
        canvas_draw_str(canvas, 14, h - 4, "Spamming");

        canvas_draw_str(canvas, w - 40, h - 4, "OK:Stop");
    } else {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 4, 28, "Press OK to start");
        canvas_draw_str(canvas, 4, 40, "Spams BLE devices");
        canvas_draw_str(canvas, 4, 52, "via built-in radio");
    }
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

View* ble_spam_view_alloc(App* app) {
    View* view = view_alloc();
    view_set_draw_callback(view, draw_callback);
    view_set_input_callback(view, input_callback);
    view_set_context(view, app);
    return view;
}

void ble_spam_view_update(App* app) {
    app->anim_frame++;
    view_dispatcher_send_custom_event(app->view_dispatcher, BLEEventUpdateDisplay);
}
