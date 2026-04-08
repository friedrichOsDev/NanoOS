/**
 * @file layouts.c
 * @author friedrichOsDev
 */

#include <layouts.h>
#include <serial.h>

/**
 * @brief Comprehensive mapping of virtual keys to their physical scancode sequences.
 * 
 * This structure provides the reverse mapping used to identify which virtual key 
 * corresponds to a specific sequence of bytes received from the keyboard controller.
 */
const scancode_to_vk_map_t scancode_to_vk_map = {
    .sc_to_vk = {
        [VK_ESCAPE] = {VK_ESCAPE, {1, SCANCODE_PREFIX_NONE, {0x01}}},
        [VK_F1] = {VK_F1, {1, SCANCODE_PREFIX_NONE, {0x3B}}},
        [VK_F2] = {VK_F2, {1, SCANCODE_PREFIX_NONE, {0x3C}}},
        [VK_F3] = {VK_F3, {1, SCANCODE_PREFIX_NONE, {0x3D}}},
        [VK_F4] = {VK_F4, {1, SCANCODE_PREFIX_NONE, {0x3E}}},
        [VK_F5] = {VK_F5, {1, SCANCODE_PREFIX_NONE, {0x3F}}},
        [VK_F6] = {VK_F6, {1, SCANCODE_PREFIX_NONE, {0x40}}},
        [VK_F7] = {VK_F7, {1, SCANCODE_PREFIX_NONE, {0x41}}},
        [VK_F8] = {VK_F8, {1, SCANCODE_PREFIX_NONE, {0x42}}},
        [VK_F9] = {VK_F9, {1, SCANCODE_PREFIX_NONE, {0x43}}},
        [VK_F10] = {VK_F10, {1, SCANCODE_PREFIX_NONE, {0x44}}},
        [VK_F11] = {VK_F11, {1, SCANCODE_PREFIX_NONE, {0x57}}},
        [VK_F12] = {VK_F12, {1, SCANCODE_PREFIX_NONE, {0x58}}},
        [VK_SNAPSHOT] = {VK_SNAPSHOT, {3, SCANCODE_PREFIX_E0, {0x2A, 0xE0, 0x37}}},
        [VK_SNAPSHOT_CONTROL] = {VK_SNAPSHOT_CONTROL, {1, SCANCODE_PREFIX_E0, {0x37}}},
        [VK_SNAPSHOT_SHIFT] = {VK_SNAPSHOT_SHIFT, {1, SCANCODE_PREFIX_E0, {0x37}}},
        [VK_SNAPSHOT_ALT] = {VK_SNAPSHOT_ALT, {1, SCANCODE_PREFIX_NONE, {0x54}}},
        [VK_SCROLL] = {VK_SCROLL, {1, SCANCODE_PREFIX_NONE, {0x46}}},
        [VK_PAUSE] = {VK_PAUSE, {5, SCANCODE_PREFIX_E1, {0x1D, 0x45, 0xE1, 0x9D, 0xC5}}},
        [VK_PAUSE_CONTROL] = {VK_PAUSE_CONTROL, {3, SCANCODE_PREFIX_E0, {0x46, 0xE0, 0xC6}}},
        [VK_OEM_3] = {VK_OEM_3, {1, SCANCODE_PREFIX_NONE, {0x29}}},
        [VK_1] = {VK_1, {1, SCANCODE_PREFIX_NONE, {0x02}}},
        [VK_2] = {VK_2, {1, SCANCODE_PREFIX_NONE, {0x03}}},
        [VK_3] = {VK_3, {1, SCANCODE_PREFIX_NONE, {0x04}}},
        [VK_4] = {VK_4, {1, SCANCODE_PREFIX_NONE, {0x05}}},
        [VK_5] = {VK_5, {1, SCANCODE_PREFIX_NONE, {0x06}}},
        [VK_6] = {VK_6, {1, SCANCODE_PREFIX_NONE, {0x07}}},
        [VK_7] = {VK_7, {1, SCANCODE_PREFIX_NONE, {0x08}}},
        [VK_8] = {VK_8, {1, SCANCODE_PREFIX_NONE, {0x09}}},
        [VK_9] = {VK_9, {1, SCANCODE_PREFIX_NONE, {0x0A}}},
        [VK_0] = {VK_0, {1, SCANCODE_PREFIX_NONE, {0x0B}}},
        [VK_OEM_MINUS] = {VK_OEM_MINUS, {1, SCANCODE_PREFIX_NONE, {0x0C}}},
        [VK_OEM_PLUS] = {VK_OEM_PLUS, {1, SCANCODE_PREFIX_NONE, {0x0D}}},
        [VK_BACK] = {VK_BACK, {1, SCANCODE_PREFIX_NONE, {0x0E}}},
        [VK_TAB] = {VK_TAB, {1, SCANCODE_PREFIX_NONE, {0x0F}}},
        [VK_Q] = {VK_Q, {1, SCANCODE_PREFIX_NONE, {0x10}}},
        [VK_W] = {VK_W, {1, SCANCODE_PREFIX_NONE, {0x11}}},
        [VK_E] = {VK_E, {1, SCANCODE_PREFIX_NONE, {0x12}}},
        [VK_R] = {VK_R, {1, SCANCODE_PREFIX_NONE, {0x13}}},
        [VK_T] = {VK_T, {1, SCANCODE_PREFIX_NONE, {0x14}}},
        [VK_Y] = {VK_Y, {1, SCANCODE_PREFIX_NONE, {0x15}}},
        [VK_U] = {VK_U, {1, SCANCODE_PREFIX_NONE, {0x16}}},
        [VK_I] = {VK_I, {1, SCANCODE_PREFIX_NONE, {0x17}}},
        [VK_O] = {VK_O, {1, SCANCODE_PREFIX_NONE, {0x18}}},
        [VK_P] = {VK_P, {1, SCANCODE_PREFIX_NONE, {0x19}}},
        [VK_OEM_4] = {VK_OEM_4, {1, SCANCODE_PREFIX_NONE, {0x1A}}},
        [VK_OEM_6] = {VK_OEM_6, {1, SCANCODE_PREFIX_NONE, {0x1B}}},
        [VK_RETURN] = {VK_RETURN, {1, SCANCODE_PREFIX_NONE, {0x1C}}},
        [VK_CAPITAL] = {VK_CAPITAL, {1, SCANCODE_PREFIX_NONE, {0x3A}}},
        [VK_A] = {VK_A, {1, SCANCODE_PREFIX_NONE, {0x1E}}},
        [VK_S] = {VK_S, {1, SCANCODE_PREFIX_NONE, {0x1F}}},
        [VK_D] = {VK_D, {1, SCANCODE_PREFIX_NONE, {0x20}}},
        [VK_F] = {VK_F, {1, SCANCODE_PREFIX_NONE, {0x21}}},
        [VK_G] = {VK_G, {1, SCANCODE_PREFIX_NONE, {0x22}}},
        [VK_H] = {VK_H, {1, SCANCODE_PREFIX_NONE, {0x23}}},
        [VK_J] = {VK_J, {1, SCANCODE_PREFIX_NONE, {0x24}}},
        [VK_K] = {VK_K, {1, SCANCODE_PREFIX_NONE, {0x25}}},
        [VK_L] = {VK_L, {1, SCANCODE_PREFIX_NONE, {0x26}}},
        [VK_OEM_1] = {VK_OEM_1, {1, SCANCODE_PREFIX_NONE, {0x27}}},
        [VK_OEM_7] = {VK_OEM_7, {1, SCANCODE_PREFIX_NONE, {0x28}}},
        [VK_OEM_5] = {VK_OEM_5, {1, SCANCODE_PREFIX_NONE, {0x2B}}},
        [VK_LSHIFT] = {VK_LSHIFT, {1, SCANCODE_PREFIX_NONE, {0x2A}}},
        [VK_OEM_102] = {VK_OEM_102, {1, SCANCODE_PREFIX_NONE, {0x56}}},
        [VK_Z] = {VK_Z, {1, SCANCODE_PREFIX_NONE, {0x2C}}},
        [VK_X] = {VK_X, {1, SCANCODE_PREFIX_NONE, {0x2D}}},
        [VK_C] = {VK_C, {1, SCANCODE_PREFIX_NONE, {0x2E}}},
        [VK_V] = {VK_V, {1, SCANCODE_PREFIX_NONE, {0x2F}}},
        [VK_B] = {VK_B, {1, SCANCODE_PREFIX_NONE, {0x30}}},
        [VK_N] = {VK_N, {1, SCANCODE_PREFIX_NONE, {0x31}}},
        [VK_M] = {VK_M, {1, SCANCODE_PREFIX_NONE, {0x32}}},
        [VK_OEM_COMMA] = {VK_OEM_COMMA, {1, SCANCODE_PREFIX_NONE, {0x33}}},
        [VK_OEM_PERIOD] = {VK_OEM_PERIOD, {1, SCANCODE_PREFIX_NONE, {0x34}}},
        [VK_OEM_2] = {VK_OEM_2, {1, SCANCODE_PREFIX_NONE, {0x35}}},
        [VK_RSHIFT] = {VK_RSHIFT, {1, SCANCODE_PREFIX_NONE, {0x36}}},
        [VK_LCONTROL] = {VK_LCONTROL, {1, SCANCODE_PREFIX_NONE, {0x1D}}},
        [VK_LWIN] = {VK_LWIN, {1, SCANCODE_PREFIX_E0, {0x5B}}},
        [VK_LMENU] = {VK_LMENU, {1, SCANCODE_PREFIX_NONE, {0x38}}},
        [VK_SPACE] = {VK_SPACE, {1, SCANCODE_PREFIX_NONE, {0x39}}},
        [VK_RMENU] = {VK_RMENU, {1, SCANCODE_PREFIX_E0, {0x38}}},
        [VK_RWIN] = {VK_RWIN, {1, SCANCODE_PREFIX_E0, {0x5C}}},
        [VK_APPS] = {VK_APPS, {1, SCANCODE_PREFIX_E0, {0x5D}}},
        [VK_RCONTROL] = {VK_RCONTROL, {1, SCANCODE_PREFIX_E0, {0x1D}}},
        [VK_INSERT] = {VK_INSERT, {1, SCANCODE_PREFIX_E0, {0x52}}},
        [VK_HOME] = {VK_HOME, {1, SCANCODE_PREFIX_E0, {0x47}}},
        [VK_PRIOR] = {VK_PRIOR, {1, SCANCODE_PREFIX_E0, {0x49}}},
        [VK_DELETE] = {VK_DELETE, {1, SCANCODE_PREFIX_E0, {0x53}}},
        [VK_END] = {VK_END, {1, SCANCODE_PREFIX_E0, {0x4F}}},
        [VK_NEXT] = {VK_NEXT, {1, SCANCODE_PREFIX_E0, {0x51}}},
        [VK_LEFT] = {VK_LEFT, {1, SCANCODE_PREFIX_E0, {0x4B}}},
        [VK_UP] = {VK_UP, {1, SCANCODE_PREFIX_E0, {0x48}}},
        [VK_RIGHT] = {VK_RIGHT, {1, SCANCODE_PREFIX_E0, {0x4D}}},
        [VK_DOWN] = {VK_DOWN, {1, SCANCODE_PREFIX_E0, {0x50}}},
        [VK_NUMLOCK] = {VK_NUMLOCK, {1, SCANCODE_PREFIX_NONE, {0x45}}},
        [VK_DIVIDE] = {VK_DIVIDE, {1, SCANCODE_PREFIX_E0, {0x35}}},
        [VK_MULTIPLY] = {VK_MULTIPLY, {1, SCANCODE_PREFIX_NONE, {0x37}}},
        [VK_SUBTRACT] = {VK_SUBTRACT, {1, SCANCODE_PREFIX_NONE, {0x4A}}},
        [VK_NUM_HOME] = {VK_NUM_HOME, {1, SCANCODE_PREFIX_NONE, {0x47}}},
        [VK_NUM_UP] = {VK_NUM_UP, {1, SCANCODE_PREFIX_NONE, {0x48}}},
        [VK_NUM_PRIOR] = {VK_NUM_PRIOR, {1, SCANCODE_PREFIX_NONE, {0x49}}},
        [VK_ADD] = {VK_ADD, {1, SCANCODE_PREFIX_NONE, {0x4E}}},
        [VK_NUM_LEFT] = {VK_NUM_LEFT, {1, SCANCODE_PREFIX_NONE, {0x4B}}},
        [VK_NUM_CLEAR] = {VK_NUM_CLEAR, {1, SCANCODE_PREFIX_NONE, {0x4C}}},
        [VK_NUM_RIGHT] = {VK_NUM_RIGHT, {1, SCANCODE_PREFIX_NONE, {0x4D}}},
        [VK_NUM_END] = {VK_NUM_END, {1, SCANCODE_PREFIX_NONE, {0x4F}}},
        [VK_NUM_DOWN] = {VK_NUM_DOWN, {1, SCANCODE_PREFIX_NONE, {0x50}}},
        [VK_NUM_NEXT] = {VK_NUM_NEXT, {1, SCANCODE_PREFIX_NONE, {0x51}}},
        [VK_NUM_INSERT] = {VK_NUM_INSERT, {1, SCANCODE_PREFIX_NONE, {0x52}}},
        [VK_NUM_DELETE] = {VK_NUM_DELETE, {1, SCANCODE_PREFIX_NONE, {0x53}}},
        [VK_NUM_RETURN] = {VK_NUM_RETURN, {1, SCANCODE_PREFIX_E0, {0x1C}}}
    }
};

/**
 * @brief Quick lookup table for single-byte scancodes (no prefix).
 * 
 * Maps a single scancode byte directly to its corresponding virtual key.
 */
const uint8_t scancode_to_vk_quick[128] = {
    [0x01] = VK_ESCAPE,
    [0x02] = VK_1,
    [0x03] = VK_2,
    [0x04] = VK_3,
    [0x05] = VK_4,
    [0x06] = VK_5,
    [0x07] = VK_6,
    [0x08] = VK_7,
    [0x09] = VK_8,
    [0x0A] = VK_9,
    [0x0B] = VK_0,
    [0x0C] = VK_OEM_MINUS,
    [0x0D] = VK_OEM_PLUS,
    [0x0E] = VK_BACK,
    [0x0F] = VK_TAB,
    [0x10] = VK_Q,
    [0x11] = VK_W,
    [0x12] = VK_E,
    [0x13] = VK_R,
    [0x14] = VK_T,
    [0x15] = VK_Y,
    [0x16] = VK_U,
    [0x17] = VK_I,
    [0x18] = VK_O,
    [0x19] = VK_P,
    [0x1A] = VK_OEM_4,
    [0x1B] = VK_OEM_6,
    [0x1C] = VK_RETURN,
    [0x1D] = VK_LCONTROL,
    [0x1E] = VK_A,
    [0x1F] = VK_S,
    [0x20] = VK_D,
    [0x21] = VK_F,
    [0x22] = VK_G,
    [0x23] = VK_H,
    [0x24] = VK_J,
    [0x25] = VK_K,
    [0x26] = VK_L,
    [0x27] = VK_OEM_1,
    [0x28] = VK_OEM_7,
    [0x29] = VK_OEM_3,
    [0x2A] = VK_LSHIFT,
    [0x2B] = VK_OEM_5,
    [0x2C] = VK_Z,
    [0x2D] = VK_X,
    [0x2E] = VK_C,
    [0x2F] = VK_V,
    [0x30] = VK_B,
    [0x31] = VK_N,
    [0x32] = VK_M,
    [0x33] = VK_OEM_COMMA,
    [0x34] = VK_OEM_PERIOD,
    [0x35] = VK_OEM_2,
    [0x36] = VK_RSHIFT,
    [0x37] = VK_MULTIPLY,
    [0x38] = VK_LMENU,
    [0x39] = VK_SPACE,
    [0x3A] = VK_CAPITAL,
    [0x3B] = VK_F1,
    [0x3C] = VK_F2,
    [0x3D] = VK_F3,
    [0x3E] = VK_F4,
    [0x3F] = VK_F5,
    [0x40] = VK_F6,
    [0x41] = VK_F7,
    [0x42] = VK_F8,
    [0x43] = VK_F9,
    [0x44] = VK_F10,
    [0x45] = VK_NUMLOCK,
    [0x46] = VK_SCROLL,
    [0x47] = VK_NUM_HOME,
    [0x48] = VK_NUM_UP,
    [0x49] = VK_NUM_PRIOR,
    [0x4A] = VK_SUBTRACT,
    [0x4B] = VK_NUM_LEFT,
    [0x4C] = VK_NUM_CLEAR,
    [0x4D] = VK_NUM_RIGHT,
    [0x4E] = VK_ADD,
    [0x4F] = VK_NUM_END,
    [0x50] = VK_NUM_DOWN,
    [0x51] = VK_NUM_NEXT,
    [0x52] = VK_NUM_INSERT,
    [0x53] = VK_NUM_DELETE,
    [0x54] = VK_SNAPSHOT_ALT,
    [0x56] = VK_OEM_102,
    [0x57] = VK_F11,
    [0x58] = VK_F12
};

/**
 * @brief Quick lookup table for two-byte scancodes with E0 prefix.
 * 
 * Maps the second byte of an E0-prefixed sequence to its corresponding virtual key.
 */
const uint8_t scancode_e0_to_vk_quick[128] = {
    [0x1C] = VK_NUM_RETURN,
    [0x1D] = VK_RCONTROL,
    [0x35] = VK_DIVIDE,
    [0x46] = VK_PAUSE_CONTROL,
    [0x37] = VK_SNAPSHOT,
    [0x38] = VK_RMENU,
    [0x47] = VK_HOME,
    [0x48] = VK_UP,
    [0x49] = VK_PRIOR,
    [0x4B] = VK_LEFT,
    [0x4D] = VK_RIGHT,
    [0x4F] = VK_END,
    [0x50] = VK_DOWN,
    [0x51] = VK_NEXT,
    [0x52] = VK_INSERT,
    [0x53] = VK_DELETE,
    [0x5B] = VK_LWIN,
    [0x5C] = VK_RWIN,
    [0x5D] = VK_APPS
};

/**
 * @brief Map indicating whether a virtual key represents an alphabetic character.
 * 
 * Used to determine if Caps Lock should affect the shift state of the key.
 */
const bool vk_is_alpha_map[VK_COUNT] = {
    [VK_A] = true, [VK_B] = true, [VK_C] = true, [VK_D] = true,
    [VK_E] = true, [VK_F] = true, [VK_G] = true, [VK_H] = true,
    [VK_I] = true, [VK_J] = true, [VK_K] = true, [VK_L] = true,
    [VK_M] = true, [VK_N] = true, [VK_O] = true, [VK_P] = true,
    [VK_Q] = true, [VK_R] = true, [VK_S] = true, [VK_T] = true,
    [VK_U] = true, [VK_V] = true, [VK_W] = true, [VK_X] = true,
    [VK_Y] = true, [VK_Z] = true
};

/**
 * @brief German keyboard layout mapping virtual keys to Unicode characters.
 * 
 * This structure defines how virtual keys are translated into characters for the German (DE) locale.
 */
const keyboard_layout_t vk_to_unicode_de = {
    .lang = "de",
    .vk_to_unicode = {
        // format: {vk, {is_special, is_num_key, is_dead_key}, normal, shift, alt, alt_gr, ctrl}
        [VK_ESCAPE] = {VK_ESCAPE, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F1] = {VK_F1, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F2] = {VK_F2, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F3] = {VK_F3, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F4] = {VK_F4, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F5] = {VK_F5, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F6] = {VK_F6, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F7] = {VK_F7, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F8] = {VK_F8, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F9] = {VK_F9, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F10] = {VK_F10, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F11] = {VK_F11, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_F12] = {VK_F12, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_SNAPSHOT] = {VK_SNAPSHOT, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_SNAPSHOT_CONTROL] = {VK_SNAPSHOT_CONTROL, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_SNAPSHOT_SHIFT] = {VK_SNAPSHOT_SHIFT, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_SNAPSHOT_ALT] = {VK_SNAPSHOT_ALT, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_SCROLL] = {VK_SCROLL, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_PAUSE] = {VK_PAUSE, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_PAUSE_CONTROL] = {VK_PAUSE_CONTROL, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_OEM_3] = {VK_OEM_3, {0, 0, 1}, 0x005E, 0x00B0, 0, 0, 0},
        [VK_1] = {VK_1, {0, 0, 0}, 0x0031, 0x0021, 0, 0, 0},
        [VK_2] = {VK_2, {0, 0, 0}, 0x0032, 0x0022, 0x00B2, 0, 0},
        [VK_3] = {VK_3, {0, 0, 0}, 0x0033, 0x00A7, 0x00B3, 0, 0},
        [VK_4] = {VK_4, {0, 0, 0}, 0x0034, 0x0024, 0, 0, 0},
        [VK_5] = {VK_5, {0, 0, 0}, 0x0035, 0x0025, 0, 0, 0},
        [VK_6] = {VK_6, {0, 0, 0}, 0x0036, 0x0026, 0, 0, 0},
        [VK_7] = {VK_7, {0, 0, 0}, 0x0037, 0x002F, 0x007B, 0, 0},
        [VK_8] = {VK_8, {0, 0, 0}, 0x0038, 0x0028, 0x005B, 0, 0},
        [VK_9] = {VK_9, {0, 0, 0}, 0x0039, 0x0029, 0x005D, 0, 0},
        [VK_0] = {VK_0, {0, 0, 0}, 0x0030, 0x003D, 0x007D, 0, 0},
        [VK_OEM_MINUS] = {VK_OEM_MINUS, {0, 0, 0}, 0x00DF, 0x003F, 0x005C, 0, 0},
        [VK_OEM_PLUS] = {VK_OEM_PLUS, {0, 0, 1}, 0x00B4, 0x0060, 0, 0, 0},
        [VK_BACK] = {VK_BACK, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_TAB] = {VK_TAB, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_Q] = {VK_Q, {0, 0, 0}, 0x0071, 0x0051, 0x0040, 0, 0},
        [VK_W] = {VK_W, {0, 0, 0}, 0x0077, 0x0057, 0, 0, 0},
        [VK_E] = {VK_E, {0, 0, 0}, 0x0065, 0x0045, 0x20AC, 0, 0},
        [VK_R] = {VK_R, {0, 0, 0}, 0x0072, 0x0052, 0, 0, 0},
        [VK_T] = {VK_T, {0, 0, 0}, 0x0074, 0x0054, 0, 0, 0},
        [VK_Y] = {VK_Y, {0, 0, 0}, 0x007A, 0x005A, 0, 0, 0},
        [VK_U] = {VK_U, {0, 0, 0}, 0x0075, 0x0055, 0, 0, 0},
        [VK_I] = {VK_I, {0, 0, 0}, 0x0069, 0x0049, 0, 0, 0},
        [VK_O] = {VK_O, {0, 0, 0}, 0x006F, 0x004F, 0, 0, 0},
        [VK_P] = {VK_P, {0, 0, 0}, 0x0070, 0x0050, 0, 0, 0},
        [VK_OEM_4] = {VK_OEM_4, {0, 0, 0}, 0x00FC, 0x00DC, 0, 0, 0},
        [VK_OEM_6] = {VK_OEM_6, {0, 0, 0}, 0x002B, 0x002A, 0x007E, 0, 0},
        [VK_RETURN] = {VK_RETURN, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_CAPITAL] = {VK_CAPITAL, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_A] = {VK_A, {0, 0, 0}, 0x0061, 0x0041, 0, 0, 0},
        [VK_S] = {VK_S, {0, 0, 0}, 0x0073, 0x0053, 0, 0, 0},
        [VK_D] = {VK_D, {0, 0, 0}, 0x0064, 0x0044, 0, 0, 0},
        [VK_F] = {VK_F, {0, 0, 0}, 0x0066, 0x0046, 0, 0, 0},
        [VK_G] = {VK_G, {0, 0, 0}, 0x0067, 0x0047, 0, 0, 0},
        [VK_H] = {VK_H, {0, 0, 0}, 0x0068, 0x0048, 0, 0, 0},
        [VK_J] = {VK_J, {0, 0, 0}, 0x006A, 0x004A, 0, 0, 0},
        [VK_K] = {VK_K, {0, 0, 0}, 0x006B, 0x004B, 0, 0, 0},
        [VK_L] = {VK_L, {0, 0, 0}, 0x006C, 0x004C, 0, 0, 0},
        [VK_OEM_1] = {VK_OEM_1, {0, 0, 0}, 0x00F6, 0x00D6, 0, 0, 0},
        [VK_OEM_7] = {VK_OEM_7, {0, 0, 0}, 0x00E4, 0x00C4, 0, 0, 0},
        [VK_OEM_5] = {VK_OEM_5, {0, 0, 0}, 0x0023, 0x0027, 0, 0, 0},
        [VK_LSHIFT] = {VK_LSHIFT, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_OEM_102] = {VK_OEM_102, {0, 0, 0}, 0x003C, 0x003E, 0x007C, 0, 0},
        [VK_Z] = {VK_Z, {0, 0, 0}, 0x0079, 0x0059, 0, 0, 0},
        [VK_X] = {VK_X, {0, 0, 0}, 0x0078, 0x0058, 0, 0, 0},
        [VK_C] = {VK_C, {0, 0, 0}, 0x0063, 0x0043, 0, 0, 0},
        [VK_V] = {VK_V, {0, 0, 0}, 0x0076, 0x0056, 0, 0, 0},
        [VK_B] = {VK_B, {0, 0, 0}, 0x0062, 0x0042, 0, 0, 0},
        [VK_N] = {VK_N, {0, 0, 0}, 0x006E, 0x004E, 0, 0, 0},
        [VK_M] = {VK_M, {0, 0, 0}, 0x006D, 0x004D, 0x00B5, 0, 0},
        [VK_OEM_COMMA] = {VK_OEM_COMMA, {0, 0, 0}, 0x002C, 0x003B, 0, 0, 0},
        [VK_OEM_PERIOD] = {VK_OEM_PERIOD, {0, 0, 0}, 0x002E, 0x003A, 0, 0, 0},
        [VK_OEM_2] = {VK_OEM_2, {0, 0, 0}, 0x002D, 0x005F, 0, 0, 0},
        [VK_RSHIFT] = {VK_RSHIFT, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_LCONTROL] = {VK_LCONTROL, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_LWIN] = {VK_LWIN, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_LMENU] = {VK_LMENU, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_SPACE] = {VK_SPACE, {0, 0, 0}, 0x0020, 0x0020, 0, 0, 0},
        [VK_RMENU] = {VK_RMENU, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_RWIN] = {VK_RWIN, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_APPS] = {VK_APPS, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_RCONTROL] = {VK_RCONTROL, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_INSERT] = {VK_INSERT, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_HOME] = {VK_HOME, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_PRIOR] = {VK_PRIOR, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_DELETE] = {VK_DELETE, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_END] = {VK_END, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_NEXT] = {VK_NEXT, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_LEFT] = {VK_LEFT, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_UP] = {VK_UP, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_RIGHT] = {VK_RIGHT, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_DOWN] = {VK_DOWN, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_NUMLOCK] = {VK_NUMLOCK, {1, 0, 0}, 0, 0, 0, 0, 0},
        [VK_DIVIDE] = {VK_DIVIDE, {0, 0, 0}, 0x002F, 0x002F, 0, 0, 0},
        [VK_MULTIPLY] = {VK_MULTIPLY, {0, 0, 0}, 0x002A, 0x002A, 0, 0, 0},
        [VK_SUBTRACT] = {VK_SUBTRACT, {0, 0, 0}, 0x002D, 0x002D, 0, 0, 0},
        [VK_NUM_HOME] = {VK_NUM_HOME, {1, 1, 0}, 0, 0, 0, 0, 0x0037},
        [VK_NUM_UP] = {VK_NUM_UP, {1, 1, 0}, 0, 0, 0, 0, 0x0038},
        [VK_NUM_PRIOR] = {VK_NUM_PRIOR, {1, 1, 0}, 0, 0, 0, 0, 0x0039},
        [VK_ADD] = {VK_ADD, {0, 0, 0}, 0x002B, 0x002B, 0, 0, 0},
        [VK_NUM_LEFT] = {VK_NUM_LEFT, {1, 1, 0}, 0, 0, 0, 0, 0x0034},
        [VK_NUM_CLEAR] = {VK_NUM_CLEAR, {1, 1, 0}, 0, 0, 0, 0, 0x0035},
        [VK_NUM_RIGHT] = {VK_NUM_RIGHT, {1, 1, 0}, 0, 0, 0, 0, 0x0036},
        [VK_NUM_END] = {VK_NUM_END, {1, 1, 0}, 0, 0, 0, 0, 0x0031},
        [VK_NUM_DOWN] = {VK_NUM_DOWN, {1, 1, 0}, 0, 0, 0, 0, 0x0032},
        [VK_NUM_NEXT] = {VK_NUM_NEXT, {1, 1, 0}, 0, 0, 0, 0, 0x0033},
        [VK_NUM_INSERT] = {VK_NUM_INSERT, {1, 1, 0}, 0, 0, 0, 0, 0x0030},
        [VK_NUM_DELETE] = {VK_NUM_DELETE, {1, 1, 0}, 0, 0, 0, 0, 0x002C},
        [VK_NUM_RETURN] = {VK_NUM_RETURN, {1, 0, 0}, 0, 0, 0, 0, 0}
    }
};

/**
 * @brief Table defining combinations of dead keys and base characters.
 * 
 * This table is used to look up the resulting Unicode character when a 
 * dead key (like ^ or `) is followed by a compatible base character (like a, e, i, o, u).
 */
const dead_key_entry_t dead_key_table[] = {
    // Circumflex (^)
    {0x005E, 0x0061, 0x00E2}, // â
    {0x005E, 0x0065, 0x00EA}, // ê
    {0x005E, 0x0069, 0x00EE}, // î
    {0x005E, 0x006F, 0x00F4}, // ô
    {0x005E, 0x0075, 0x00FB}, // û
    {0x005E, 0x0041, 0x00C2}, // Â
    {0x005E, 0x0045, 0x00CA}, // Ê
    {0x005E, 0x0049, 0x00CE}, // Î
    {0x005E, 0x004F, 0x00D4}, // Ô
    {0x005E, 0x0055, 0x00DB}, // Û

    // Grave (`)
    {0x0060, 0x0061, 0x00E0}, // à
    {0x0060, 0x0065, 0x00E8}, // è
    {0x0060, 0x0069, 0x00EC}, // ì
    {0x0060, 0x006F, 0x00F2}, // ò
    {0x0060, 0x0075, 0x00F9}, // ù
    {0x0060, 0x0041, 0x00C0}, // À
    {0x0060, 0x0045, 0x00C8}, // È
    {0x0060, 0x0049, 0x00CC}, // Ì
    {0x0060, 0x004F, 0x00D2}, // Ò
    {0x0060, 0x0055, 0x00D9}, // Ù

    // Acute (´)
    {0x00B4, 0x0061, 0x00E1}, // á
    {0x00B4, 0x0065, 0x00E9}, // é
    {0x00B4, 0x0069, 0x00ED}, // í
    {0x00B4, 0x006F, 0x00F3}, // ó
    {0x00B4, 0x0075, 0x00FA}, // ú
    {0x00B4, 0x0079, 0x00FD}, // ý
    {0x00B4, 0x0041, 0x00C1}, // Á
    {0x00B4, 0x0045, 0x00C9}, // É
    {0x00B4, 0x0049, 0x00CD}, // Í
    {0x00B4, 0x004F, 0x00D3}, // Ó
    {0x00B4, 0x0055, 0x00DA}, // Ú
    {0x00B4, 0x0059, 0x00DD}, // Ý

    // Terminator
    {0, 0, 0}
};
