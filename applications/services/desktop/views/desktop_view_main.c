#include <gui/gui_i.h>
#include <gui/view.h>
#include <gui/elements.h>
#include <gui/canvas.h>
#include <furi.h>
#include <input/input.h>
#include <dolphin/dolphin.h>

#include "desktop_view_main.h"

struct DesktopMainView {
    View* view;
    DesktopMainViewCallback callback;
    void* context;
    FuriTimer* poweroff_timer;
    FuriTimer* left_short_timer;
    uint32_t last_left_press_tick;
    bool dummy_mode;
};

#define DESKTOP_MAIN_VIEW_POWEROFF_TIMEOUT 1300
#define DESKTOP_MAIN_VIEW_DOUBLE_PRESS_DELAY 300

static void desktop_main_poweroff_timer_callback(void* context) {
    DesktopMainView* main_view = context;
    main_view->callback(DesktopMainEventOpenPowerOff, main_view->context);
}

static void desktop_main_left_short_timer_callback(void* context) {
    DesktopMainView* main_view = context;
    main_view->callback(DesktopMainEventOpenFavoriteLeftShort, main_view->context);
}

void desktop_main_set_callback(
    DesktopMainView* main_view,
    DesktopMainViewCallback callback,
    void* context) {
    furi_assert(main_view);
    furi_assert(callback);
    main_view->callback = callback;
    main_view->context = context;
}

View* desktop_main_get_view(DesktopMainView* main_view) {
    furi_assert(main_view);
    return main_view->view;
}

void desktop_main_set_dummy_mode_state(DesktopMainView* main_view, bool dummy_mode) {
    furi_assert(main_view);
    main_view->dummy_mode = dummy_mode;
}

bool desktop_main_input_callback(InputEvent* event, void* context) {
    furi_assert(event);
    furi_assert(context);

    DesktopMainView* main_view = context;

    if(main_view->dummy_mode == false) {
        if(event->type == InputTypePress && event->key == InputKeyLeft) {
            uint32_t now = furi_get_tick();
            if(now - main_view->last_left_press_tick < DESKTOP_MAIN_VIEW_DOUBLE_PRESS_DELAY) {
                furi_timer_stop(main_view->left_short_timer);
                main_view->callback(DesktopMainEventOpenIrDoublePress, main_view->context);
            } else {
                furi_timer_start(main_view->left_short_timer, DESKTOP_MAIN_VIEW_DOUBLE_PRESS_DELAY);
            }
            main_view->last_left_press_tick = now;
        } else if(event->type == InputTypeShort) {
            if(event->key == InputKeyOk) {
                main_view->callback(DesktopMainEventOpenMenu, main_view->context);
            } else if(event->key == InputKeyUp) {
                main_view->callback(DesktopMainEventOpenLockMenu, main_view->context);
            } else if(event->key == InputKeyDown) {
                main_view->callback(DesktopMainEventOpenArchive, main_view->context);
            } else if(event->key == InputKeyLeft) {
                // Consumed by Press-based timer
            }
            // Right key short is handled by animation manager
        } else if(event->type == InputTypeLong) {
            if(event->key == InputKeyUp) {
                main_view->callback(DesktopMainEventLock, main_view->context);
            } else if(event->key == InputKeyDown) {
                main_view->callback(DesktopMainEventOpenDebug, main_view->context);
            } else if(event->key == InputKeyLeft) {
                furi_timer_stop(main_view->left_short_timer);
                main_view->callback(DesktopMainEventOpenFavoriteLeftLong, main_view->context);
            } else if(event->key == InputKeyRight) {
                main_view->callback(DesktopMainEventOpenFavoriteRightLong, main_view->context);
            } else if(event->key == InputKeyOk) {
                if(furi_hal_rtc_is_flag_set(FuriHalRtcFlagDebug)) {
                    main_view->callback(DesktopAnimationEventNewIdleAnimation, main_view->context);
                } else {
                    main_view->callback(DesktopMainEventOpenFavoriteOkLong, main_view->context);
                }
            }
        }
    } else {
        if(event->type == InputTypeShort) {
            if(event->key == InputKeyOk) {
                main_view->callback(DesktopDummyEventOpenOk, main_view->context);
            } else if(event->key == InputKeyUp) {
                main_view->callback(DesktopMainEventOpenLockMenu, main_view->context);
            } else if(event->key == InputKeyDown) {
                main_view->callback(DesktopDummyEventOpenDown, main_view->context);
            } else if(event->key == InputKeyLeft) {
                main_view->callback(DesktopDummyEventOpenLeft, main_view->context);
            }
            // Right key short is handled by animation manager
        } else if(event->type == InputTypeLong) {
            if(event->key == InputKeyOk) {
                // Not working in DummyMode
                // if(furi_hal_rtc_is_flag_set(FuriHalRtcFlagDebug)) {
                //     main_view->callback(DesktopAnimationEventNewIdleAnimation, main_view->context);
                // }
                main_view->callback(DesktopDummyEventOpenOkLong, main_view->context);
            } else if(event->key == InputKeyUp) {
                main_view->callback(DesktopDummyEventOpenUpLong, main_view->context);
            } else if(event->key == InputKeyDown) {
                main_view->callback(DesktopDummyEventOpenDownLong, main_view->context);
            } else if(event->key == InputKeyLeft) {
                main_view->callback(DesktopDummyEventOpenLeftLong, main_view->context);
            } else if(event->key == InputKeyRight) {
                main_view->callback(DesktopDummyEventOpenRightLong, main_view->context);
            }
        }
    }

    if(event->key == InputKeyBack) {
        if(event->type == InputTypePress) {
            furi_timer_start(main_view->poweroff_timer, DESKTOP_MAIN_VIEW_POWEROFF_TIMEOUT);
        } else if(event->type == InputTypeRelease) {
            furi_timer_stop(main_view->poweroff_timer);
        }
    }

    return true;
}

DesktopMainView* desktop_main_alloc(void) {
    DesktopMainView* main_view = malloc(sizeof(DesktopMainView));

    main_view->view = view_alloc();
    view_set_context(main_view->view, main_view);
    view_set_input_callback(main_view->view, desktop_main_input_callback);

    main_view->poweroff_timer =
        furi_timer_alloc(desktop_main_poweroff_timer_callback, FuriTimerTypeOnce, main_view);
    main_view->left_short_timer =
        furi_timer_alloc(desktop_main_left_short_timer_callback, FuriTimerTypeOnce, main_view);
    main_view->last_left_press_tick = 0;

    return main_view;
}

void desktop_main_free(DesktopMainView* main_view) {
    furi_assert(main_view);
    view_free(main_view->view);
    furi_timer_free(main_view->poweroff_timer);
    furi_timer_free(main_view->left_short_timer);
    free(main_view);
}
