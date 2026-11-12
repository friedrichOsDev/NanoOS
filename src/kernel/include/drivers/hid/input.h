/**
 * @file input.h
 * @brief Hardware-independent keyboard and pointer API.
 * @author friedrichOsDev
 */

#pragma once

#include <drivers/hid/hid.h>

#define INPUT_LAYOUT_NAME_MAX 32
#define INPUT_KEY_COUNT ((size_t)KEY_NUMLOCK_DELETE + 1)

/** @brief Keyboard layout definition mapping keycodes to Unicode codepoints. */
typedef struct {
    const char *name;                 /**< Layout name identifier (e.g., "de-DE"). */
    uint32_t normal[INPUT_KEY_COUNT]; /**< Unmodified key codepoint mapping array. */
    uint32_t shift[INPUT_KEY_COUNT];  /**< Shift-modified key codepoint mapping array. */
    uint32_t altgr[INPUT_KEY_COUNT];  /**< AltGr-modified key codepoint mapping array. */
} keyboard_layout_t;

/** @brief Current state of the keyboard including modifier states and toggle locks. */
typedef struct {
    bool pressed[INPUT_KEY_COUNT]; /**< State array tracking currently pressed keys. */
    bool shift;                    /**< True if either Left or Right Shift is active. */
    bool control;                  /**< True if either Left or Right Control is active. */
    bool alt;                      /**< True if Left Alt (Menu) is active. */
    bool altgr;                    /**< True if Right Alt (AltGr) is active. */
    bool caps_lock;                /**< True if Caps Lock is toggled on. */
    bool num_lock;                 /**< True if Num Lock is toggled on. */
} keyboard_state_t;

/** @brief Current state of the mouse pointer position and button bitmask. */
typedef struct {
    int64_t x;       /**< Accumulated X coordinate of the mouse pointer. */
    int64_t y;       /**< Accumulated Y coordinate of the mouse pointer. */
    uint8_t buttons; /**< Bitmask of currently depressed mouse buttons. */
} mouse_state_t;

/** @brief Enumeration of key action types for translated input events. */
typedef enum {
    INPUT_KEY_ACTION_NONE = 0, /**< No key action occurred. */
    INPUT_KEY_ACTION_PRESSED,  /**< Key was pressed down for the first time. */
    INPUT_KEY_ACTION_REPEATED, /**< Key is being held, producing a typematic repeat. */
    INPUT_KEY_ACTION_RELEASED  /**< Key was released. */
} input_key_action_t;

/** @brief High-level translated input event combining raw HID data, Unicode output, and dead keys. */
typedef struct {
    hid_event_t hid;               /**< Underlying raw HID event. */
    uint32_t unicode;              /**< Primary translated Unicode codepoint. */
    uint32_t unicode_secondary;    /**< Secondary Unicode codepoint (if dual emission occurs). */
    uint8_t unicode_count;         /**< Number of valid Unicode codepoints emitted (0, 1, or 2). */
    uint32_t dead_key_accent;      /**< Active dead-key accent codepoint, if any. */
    uint32_t dead_key_base;        /**< Base character codepoint combined with a dead key. */
    uint32_t dead_key_result;      /**< Resulting precomposed or fallback codepoint from a dead key operation. */
    input_key_action_t key_action; /**< Classified key action state. */
} input_event_t;

/** @brief Input event callback registered by a kernel subsystem. */
typedef void (*input_event_handler_t)(const input_event_t *event, void *context);

kerr_t input_init();
kerr_t input_poll_event(input_event_t *event);
kerr_t input_register_handler(input_event_handler_t handler, void *context);
kerr_t input_dispatcher_start();
kerr_t keyboard_register_layout(const keyboard_layout_t *layout);
kerr_t keyboard_set_layout(const char *name);
const keyboard_layout_t *keyboard_get_layout();
uint32_t keyboard_translate_key(hid_keycode_t key, bool shift, bool altgr);
kerr_t keyboard_get_state(keyboard_state_t *out);
kerr_t mouse_get_state(mouse_state_t *out);
