/**
 * @file hid.h
 * @brief Human interface driver HAL
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

typedef enum {
    HID_EVENT_NONE = 0,
    HID_EVENT_KEY_PRESS,
    HID_EVENT_KEY_RELEASE,
    HID_EVENT_MOUSE_MOVE,
    HID_EVENT_MOUSE_BUTTON_PRESS,
    HID_EVENT_MOUSE_BUTTON_RELEASE,
    HID_EVENT_MOUSE_SCROLL
} hid_event_type_t;

typedef enum {
    KEY_UNKNOWN = 0,
    KEY_ESC,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_SNAPSHOT,
    KEY_SCROLL,
    KEY_PAUSE,
    KEY_OEM5,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_OEM4,
    KEY_OEM6,
    KEY_BACK,
    KEY_INSERT,
    KEY_HOME,
    KEY_PRIOR,
    KEY_NUMLOCK,
    KEY_NUMLOCK_DIVIDE,
    KEY_NUMLOCK_MULTIPLY,
    KEY_NUMLOCK_SUBTRACT,
    KEY_TAB,
    KEY_Q,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Z,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_OEM1,
    KEY_OEM_PLUS,
    KEY_RETURN,
    KEY_DELETE,
    KEY_END,
    KEY_NEXT,
    KEY_NUMLOCK_HOME,
    KEY_NUMLOCK_UP,
    KEY_NUMLOCK_PRIOR,
    KEY_NUMLOCK_ADD,
    KEY_CAPITAL,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_OEM3,
    KEY_OEM7,
    KEY_OEM2,
    KEY_NUMLOCK_LEFT,
    KEY_NUMLOCK_CLEAR,
    KEY_NUMLOCK_RIGHT,
    KEY_LSHIFT,
    KEY_OEM10,
    KEY_Y,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_OEM_COMMA,
    KEY_OEM_PERIOD,
    KEY_OEM_MINUS,
    KEY_RSHIFT,
    KEY_NUMLOCK_END,
    KEY_NUMLOCK_DOWN,
    KEY_NUMLOCK_NEXT,
    KEY_NUMLOCK_RETURN,
    KEY_LCONTROL,
    KEY_LWIN,
    KEY_LMENU,
    KEY_SPACE,
    KEY_RMENU,
    KEY_RWIN,
    KEY_APPS,
    KEY_RCONTROL,
    KEY_UP,
    KEY_LEFT,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_NUMLOCK_INSERT,
    KEY_NUMLOCK_DELETE,
} hid_keycode_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"
#define MOUSE_BTN_LEFT (1 << 0)
#define MOUSE_BTN_RIGHT (1 << 1)
#define MOUSE_BTN_MIDDLE (1 << 2)
#pragma GCC diagnostic pop

typedef struct {
    hid_event_type_t type;
    uint64_t timestamp;
    union {
        struct {
            hid_keycode_t keycode;
            uint8_t scancode;
        } key;
        struct { int64_t dx; int64_t dy; } mouse_move;
        struct { uint8_t button; } mouse_btn;
        struct { int64_t delta; } mouse_scroll;
    };
} hid_event_t;

typedef struct hid_driver {
    const char *driver_name;
    kerr_t (*init)();
    void (*deinit)();
    void (*enable)();
    void (*disable)();
    struct hid_driver *next;
} hid_driver_t;

kerr_t hid_init();
kerr_t hid_deinit();
kerr_t hid_register_driver(hid_driver_t *driver);
kerr_t hid_push_event(const hid_event_t *event);
kerr_t hid_poll_event(hid_event_t *out_event);
