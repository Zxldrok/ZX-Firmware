#include "scenes.h"

#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
static void (*const zx_ble_spam_on_enter_handlers[])(void*) = {
#include "scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
static bool (*const zx_ble_spam_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
static void (*const zx_ble_spam_on_exit_handlers[])(void* context) = {
#include "scene_config.h"
};
#undef ADD_SCENE

const SceneManagerHandlers zx_ble_spam_scene_handlers = {
    .on_enter_handlers = zx_ble_spam_on_enter_handlers,
    .on_event_handlers = zx_ble_spam_on_event_handlers,
    .on_exit_handlers = zx_ble_spam_on_exit_handlers,
    .scene_num = ZxBleSpamSceneNum,
};
