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
 * These are independent of the keyboard layout.
 */
typedef enum {
    VK_NONE,
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
    /**
     * @brief Total number of virtual keys.
     * 
     * This value is used to size arrays and iterate through all virtual keys.
     */
    VK_COUNT
} virtual_key_t;

/**
 * @brief Scancode prefixes used by the PS/2 keyboard controller.
 */
typedef enum {
    SCANCODE_PREFIX_NONE = 0,
    SCANCODE_PREFIX_E0 = 0xE0,
    SCANCODE_PREFIX_E1 = 0xE1,
} scancode_prefix_t;

/**
 * @brief Represents a sequence of bytes that form a physical scancode.
 */
typedef struct {
    uint8_t length;
    scancode_prefix_t prefix;
    uint8_t bytes[5];
} scancode_sequence_t;

/**
 * @brief Mapping entry from a virtual key to its scancode sequence.
 */
typedef struct {
    virtual_key_t vk;
    scancode_sequence_t sequence;
} scancode_to_vk_t;

/**
 * @brief A complete map of all virtual keys to their physical scancodes.
 */
typedef struct{
    scancode_to_vk_t sc_to_vk[VK_COUNT];
} scancode_to_vk_map_t;

/**
 * @brief Flags describing the behavior and properties of a virtual key.
 */
typedef struct {
    bool is_special; // means the key doesn't produce a character (e.g., Shift, Ctrl, F1-F12) and we need to handle it differently
    bool is_num_key;
    bool is_dead_key;
} key_flags_t;

/**
 * @brief Mapping from a virtual key to Unicode characters based on modifier states.
 */
typedef struct {
    virtual_key_t vk;
    key_flags_t flags;
    uint32_t normal;
    uint32_t shift;
    uint32_t alt_gr;
    uint32_t ctrl;
    uint32_t num;
} vk_to_unicode_t;

/**
 * @brief Represents a complete keyboard layout mapping.
 */
typedef struct {
    char lang[3]; /**< ISO 639-1 language code (e.g., "en", "de"). */
    vk_to_unicode_t vk_to_unicode[VK_COUNT]; /**< Mapping from virtual keys to Unicode characters. */
} keyboard_layout_t;

typedef struct {
    uint32_t dead_char;
    uint32_t base_char;
    uint32_t combined;
} dead_key_entry_t;

extern const scancode_to_vk_map_t scancode_to_vk_map;
extern const uint8_t scancode_to_vk_quick[128];
extern const uint8_t scancode_e0_to_vk_quick[128];
extern const bool vk_is_alpha_map[VK_COUNT];
extern const keyboard_layout_t vk_to_unicode_de;
extern const dead_key_entry_t dead_key_table[];
