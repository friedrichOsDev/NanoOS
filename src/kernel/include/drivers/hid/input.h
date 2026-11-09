/**
 * @file input.h
 * @brief Hardware-independent keyboard and pointer API.
 */
#pragma once

#include <drivers/hid/hid.h>

#define INPUT_LAYOUT_NAME_MAX 32
#define INPUT_KEY_COUNT ((size_t)KEY_NUMLOCK_DELETE + 1)

typedef struct {
    const char *name;
    /* Unicode code point produced by a physical key without modifiers. */
    uint32_t normal[INPUT_KEY_COUNT];
    uint32_t shift[INPUT_KEY_COUNT];
    uint32_t altgr[INPUT_KEY_COUNT];
} keyboard_layout_t;

typedef struct {
    bool pressed[INPUT_KEY_COUNT];
    bool shift;
    bool control;
    bool alt;
    bool altgr;
    bool caps_lock;
    bool num_lock;
} keyboard_state_t;

typedef struct {
    int64_t x;
    int64_t y;
    uint8_t buttons;
} mouse_state_t;

/**
 * @brief Semantic action for a keyboard event after key-state processing.
 *
 * A repeated action is a make event received while the key was already down.
 * This is useful for text input and key-repeat handling without maintaining
 * another pressed-key table in every consumer.
 */
typedef enum {
    INPUT_KEY_ACTION_NONE = 0,
    INPUT_KEY_ACTION_PRESSED,
    INPUT_KEY_ACTION_REPEATED,
    INPUT_KEY_ACTION_RELEASED
} input_key_action_t;

typedef struct {
    hid_event_t hid;
    /* First Unicode code point emitted by this key event, if any. */
    uint32_t unicode;
    /*
     * Optional second code point. When a pending dead key cannot be composed,
     * unicode contains the spacing accent and unicode_secondary the base key.
     * unicode_count is 0, 1, or 2. Existing consumers may continue using
     * unicode; text consumers should process both code points in order.
     */
    uint32_t unicode_secondary;
    uint8_t unicode_count;
    input_key_action_t key_action;
} input_event_t;

kerr_t input_init(void);
kerr_t input_poll_event(input_event_t *event);
kerr_t keyboard_register_layout(const keyboard_layout_t *layout);
kerr_t keyboard_set_layout(const char *name);
const keyboard_layout_t *keyboard_get_layout(void);
uint32_t keyboard_translate_key(hid_keycode_t key, bool shift, bool altgr);
kerr_t keyboard_get_state(keyboard_state_t *out);
kerr_t mouse_get_state(mouse_state_t *out);
