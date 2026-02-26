/**
 * @file layouts.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Virtual key codes representing physical keys on a keyboard.
 */
typedef enum {
    VK_ESCAPE,
    VK_F1,
    VK_F2,
    VK_F3,
    VK_F4,
    VK_F5,
    VK_F6,
    VK_F7,
    VK_F8,
    VK_F9,
    VK_F10,
    VK_F11,
    VK_F12,
    VK_SNAPSHOT,
    VK_SNAPSHOT_CONTROL,
    VK_SNAPSHOT_SHIFT,
    VK_SNAPSHOT_ALT,
    VK_SCROLL,
    VK_PAUSE,
    VK_PAUSE_CONTROL,
    VK_OEM_3,
    VK_1,
    VK_2,
    VK_3,
    VK_4,
    VK_5,
    VK_6,
    VK_7,
    VK_8,
    VK_9,
    VK_0,
    VK_OEM_MINUS,
    VK_OEM_PLUS,
    VK_BACK,
    VK_TAB,
    VK_Q,
    VK_W,
    VK_E,
    VK_R,
    VK_T,
    VK_Y,
    VK_U,
    VK_I,
    VK_O,
    VK_P,
    VK_OEM_4,
    VK_OEM_6,
    VK_RETURN,
    VK_CAPITAL,
    VK_A,
    VK_S,
    VK_D,
    VK_F,
    VK_G,
    VK_H,
    VK_J,
    VK_K,
    VK_L,
    VK_OEM_1,
    VK_OEM_7,
    VK_OEM_5,
    VK_LSHIFT,
    VK_OEM_102,
    VK_Z,
    VK_X,
    VK_C,
    VK_V,
    VK_B,
    VK_N,
    VK_M,
    VK_OEM_COMMA,
    VK_OEM_PERIOD,
    VK_OEM_2,
    VK_RSHIFT,
    VK_LCONTROL,
    VK_LWIN,
    VK_LMENU,
    VK_SPACE,
    VK_RMENU,
    VK_RWIN,
    VK_APPS,
    VK_RCONTROL,
    VK_INSERT,
    VK_HOME,
    VK_PRIOR,
    VK_DELETE,
    VK_END,
    VK_NEXT,
    VK_LEFT,
    VK_UP,
    VK_RIGHT,
    VK_DOWN,
    VK_NUMLOCK,
    VK_DIVIDE,
    VK_MULTIPLY,
    VK_SUBTRACT,
    VK_NUM_HOME,
    VK_NUM_UP,
    VK_NUM_PRIOR,
    VK_ADD,
    VK_NUM_LEFT,
    VK_NUM_CLEAR,
    VK_NUM_RIGHT,
    VK_NUM_END,
    VK_NUM_DOWN,
    VK_NUM_NEXT,
    VK_NUM_RETURN,
    VK_NUM_INSERT,
    VK_NUM_DELETE,
    VK_COUNT
} virtual_key_t;

/**
 * @brief Scancode prefixes for extended keys.
 */
typedef enum {
    SCANCODE_PREFIX_NONE = 0,
    SCANCODE_PREFIX_E0 = 0xE0,
    SCANCODE_PREFIX_E1 = 0xE1,
} scancode_prefix_t;

/**
 * @brief Represents a sequence of scancodes that identify a key press.
 */
typedef struct {
    uint8_t length;
    scancode_prefix_t prefix;
    uint8_t bytes[5];
} scancode_sequence_t;

typedef struct {
    bool is_special;
    bool is_num_key;
    char normal[5];
    char shift[5];
    char alt_gr[5];
    char num[5];
} key_mapping_t;

/**
 * @brief Maps a virtual key to its corresponding scancode sequence.
 */
typedef struct {
    virtual_key_t vk;
    scancode_sequence_t scancode_sequence;
    key_mapping_t mapping;
} key_t;

/**
 * @brief Represents a complete keyboard layout mapping.
 */
typedef struct {
    char lang[3]; /**< ISO 639-1 language code (e.g., "en", "de"). */
    key_t keys[VK_COUNT];
} keyboard_layout_t;

extern keyboard_layout_t de;

void dump_layout(keyboard_layout_t *layout);

