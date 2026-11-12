/**
 * @file hid.h
 * @brief Human interface driver HAL
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/** @brief Enumeration of raw HID event types. */
typedef enum {
    HID_EVENT_NONE = 0,             /**< No event. */
    HID_EVENT_KEY_PRESS,            /**< Key press event. */
    HID_EVENT_KEY_RELEASE,          /**< Key release event. */
    HID_EVENT_MOUSE_MOVE,           /**< Mouse movement event. */
    HID_EVENT_MOUSE_BUTTON_PRESS,   /**< Mouse button press event. */
    HID_EVENT_MOUSE_BUTTON_RELEASE, /**< Mouse button release event. */
    HID_EVENT_MOUSE_SCROLL          /**< Mouse scroll wheel event. */
} hid_event_type_t;

/** @brief Enumeration of raw HID keyboard keycodes. */
typedef enum {
    KEY_UNKNOWN = 0,      /**< Unknown or unmapped key. */
    KEY_ESC,              /**< Escape key. */
    KEY_F1,               /**< Function key F1. */
    KEY_F2,               /**< Function key F2. */
    KEY_F3,               /**< Function key F3. */
    KEY_F4,               /**< Function key F4. */
    KEY_F5,               /**< Function key F5. */
    KEY_F6,               /**< Function key F6. */
    KEY_F7,               /**< Function key F7. */
    KEY_F8,               /**< Function key F8. */
    KEY_F9,               /**< Function key F9. */
    KEY_F10,              /**< Function key F10. */
    KEY_F11,              /**< Function key F11. */
    KEY_F12,              /**< Function key F12. */
    KEY_SNAPSHOT,         /**< Print Screen / Snapshot key. */
    KEY_SCROLL,           /**< Scroll Lock key. */
    KEY_PAUSE,            /**< Pause / Break key. */
    KEY_OEM5,             /**< OEM key 5 (e.g., '^' / '°' on German keyboards). */
    KEY_1,                /**< Number key 1. */
    KEY_2,                /**< Number key 2. */
    KEY_3,                /**< Number key 3. */
    KEY_4,                /**< Number key 4. */
    KEY_5,                /**< Number key 5. */
    KEY_6,                /**< Number key 6. */
    KEY_7,                /**< Number key 7. */
    KEY_8,                /**< Number key 8. */
    KEY_9,                /**< Number key 9. */
    KEY_0,                /**< Number key 0. */
    KEY_OEM4,             /**< OEM key 4 (e.g., 'ß' / '?' on German keyboards). */
    KEY_OEM6,             /**< OEM key 6 (e.g., '´' / '`' on German keyboards). */
    KEY_BACK,             /**< Backspace key. */
    KEY_INSERT,           /**< Insert key. */
    KEY_HOME,             /**< Home key. */
    KEY_PRIOR,            /**< Page Up key. */
    KEY_NUMLOCK,          /**< Num Lock key. */
    KEY_NUMLOCK_DIVIDE,   /**< Numpad divide key ('/'). */
    KEY_NUMLOCK_MULTIPLY, /**< Numpad multiply key ('*'). */
    KEY_NUMLOCK_SUBTRACT, /**< Numpad subtract key ('-'). */
    KEY_TAB,              /**< Tab key. */
    KEY_Q,                /**< Letter Q. */
    KEY_W,                /**< Letter W. */
    KEY_E,                /**< Letter E. */
    KEY_R,                /**< Letter R. */
    KEY_T,                /**< Letter T. */
    KEY_Z,                /**< Letter Z. */
    KEY_U,                /**< Letter U. */
    KEY_I,                /**< Letter I. */
    KEY_O,                /**< Letter O. */
    KEY_P,                /**< Letter P. */
    KEY_OEM1,             /**< OEM key 1 (e.g., 'ü' / 'Ü' on German keyboards). */
    KEY_OEM_PLUS,         /**< OEM plus key ('+'). */
    KEY_RETURN,           /**< Enter / Return key. */
    KEY_DELETE,           /**< Delete key. */
    KEY_END,              /**< End key. */
    KEY_NEXT,             /**< Page Down key. */
    KEY_NUMLOCK_HOME,     /**< Numpad 7 / Home (when Num Lock is off). */
    KEY_NUMLOCK_UP,       /**< Numpad 8 / Up Arrow (when Num Lock is off). */
    KEY_NUMLOCK_PRIOR,    /**< Numpad 9 / Page Up (when Num Lock is off). */
    KEY_NUMLOCK_ADD,      /**< Numpad add key ('+'). */
    KEY_CAPITAL,          /**< Caps Lock key. */
    KEY_A,                /**< Letter A. */
    KEY_S,                /**< Letter S. */
    KEY_D,                /**< Letter D. */
    KEY_F,                /**< Letter F. */
    KEY_G,                /**< Letter G. */
    KEY_H,                /**< Letter H. */
    KEY_J,                /**< Letter J. */
    KEY_K,                /**< Letter K. */
    KEY_L,                /**< Letter L. */
    KEY_OEM3,             /**< OEM key 3 (e.g., 'ö' / 'Ö' on German keyboards). */
    KEY_OEM7,             /**< OEM key 7 (e.g., 'ä' / 'Ä' on German keyboards). */
    KEY_OEM2,             /**< OEM key 2 (e.g., '#' / ''' on German keyboards). */
    KEY_NUMLOCK_LEFT,     /**< Numpad 4 / Left Arrow (when Num Lock is off). */
    KEY_NUMLOCK_CLEAR,    /**< Numpad 5 / Clear (when Num Lock is off). */
    KEY_NUMLOCK_RIGHT,    /**< Numpad 6 / Right Arrow (when Num Lock is off). */
    KEY_LSHIFT,           /**< Left Shift key. */
    KEY_OEM10,            /**< OEM key 10 (e.g., '<' / '>' on German keyboards). */
    KEY_Y,                /**< Letter Y. */
    KEY_X,                /**< Letter X. */
    KEY_C,                /**< Letter C. */
    KEY_V,                /**< Letter V. */
    KEY_B,                /**< Letter B. */
    KEY_N,                /**< Letter N. */
    KEY_M,                /**< Letter M. */
    KEY_OEM_COMMA,        /**< OEM comma key (','). */
    KEY_OEM_PERIOD,       /**< OEM period key ('.'). */
    KEY_OEM_MINUS,        /**< OEM minus key ('-'). */
    KEY_RSHIFT,           /**< Right Shift key. */
    KEY_NUMLOCK_END,      /**< Numpad 1 / End (when Num Lock is off). */
    KEY_NUMLOCK_DOWN,     /**< Numpad 2 / Down Arrow (when Num Lock is off). */
    KEY_NUMLOCK_NEXT,     /**< Numpad 3 / Page Down (when Num Lock is off). */
    KEY_NUMLOCK_RETURN,   /**< Numpad Enter key. */
    KEY_LCONTROL,         /**< Left Control key. */
    KEY_LWIN,             /**< Left Windows / Super key. */
    KEY_LMENU,            /**< Left Alt key. */
    KEY_SPACE,            /**< Spacebar. */
    KEY_RMENU,            /**< Right Alt / AltGr key. */
    KEY_RWIN,             /**< Right Windows / Super key. */
    KEY_APPS,             /**< Application / Menu key. */
    KEY_RCONTROL,         /**< Right Control key. */
    KEY_UP,               /**< Up Arrow key. */
    KEY_LEFT,             /**< Left Arrow key. */
    KEY_DOWN,             /**< Down Arrow key. */
    KEY_RIGHT,            /**< Right Arrow key. */
    KEY_NUMLOCK_INSERT,   /**< Numpad 0 / Insert (when Num Lock is off). */
    KEY_NUMLOCK_DELETE,   /**< Numpad . / Delete (when Num Lock is off). */
} hid_keycode_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"
#define MOUSE_BTN_LEFT (1 << 0)   /**< Left mouse button bitmask flag. */
#define MOUSE_BTN_RIGHT (1 << 1)  /**< Right mouse button bitmask flag. */
#define MOUSE_BTN_MIDDLE (1 << 2) /**< Middle mouse button bitmask flag. */
#pragma GCC diagnostic pop

/** @brief Raw HID event structure containing timestamp, event type, and payload union. */
typedef struct {
    hid_event_type_t type; /**< Type of the raw HID event. */
    uint64_t timestamp;    /**< Hardware timestamp when the event occurred. */
    union {
        struct {
            hid_keycode_t keycode; /**< Mapped HID keycode. */
            uint8_t scancode;      /**< Raw hardware scancode. */
        } key;                     /**< Keyboard event payload. */
        struct {
            int64_t dx; /**< Relative displacement along the X axis. */
            int64_t dy; /**< Relative displacement along the Y axis. */
        } mouse_move;   /**< Mouse movement event payload. */
        struct {
            uint8_t button; /**< Mouse button identifier/bitmask involved. */
        } mouse_btn;        /**< Mouse button event payload. */
        struct {
            int64_t delta; /**< Scroll wheel displacement delta. */
        } mouse_scroll;    /**< Mouse scroll event payload. */
    };
} hid_event_t;

/** @brief HID driver interface structure for registering and controlling input hardware drivers. */
typedef struct hid_driver {
    const char *driver_name; /**< Null-terminated string identifying the driver. */
    kerr_t (*init)();        /**< Driver initialization callback function pointer. */
    void (*deinit)();        /**< Driver deinitialization callback function pointer. */
    void (*enable)();        /**< Driver enable/activation callback function pointer. */
    void (*disable)();       /**< Driver disable/deactivation callback function pointer. */
    struct hid_driver *next; /**< Pointer to the next registered HID driver in the linked list. */
} hid_driver_t;

kerr_t hid_init();
kerr_t hid_deinit();
kerr_t hid_register_driver(hid_driver_t *driver);
kerr_t hid_push_event(const hid_event_t *event);
kerr_t hid_poll_event(hid_event_t *out_event);
