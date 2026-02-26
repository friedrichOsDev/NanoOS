/**
 * @file layouts.c
 * @author friedrichOsDev
 */

#include <layouts.h>
#include <serial.h>

/**
 * @brief German (DE) keyboard layout definition.
 */
keyboard_layout_t de = {
    .lang = "de",
    .keys = {
        [VK_ESCAPE] = {VK_ESCAPE, {1, SCANCODE_PREFIX_NONE, {0x01}}, {true, false, "", "", "", ""}},
        [VK_F1] = {VK_F1, {1, SCANCODE_PREFIX_NONE, {0x3B}}, {true, false, "", "", "", ""}},
        [VK_F2] = {VK_F2, {1, SCANCODE_PREFIX_NONE, {0x3C}}, {true, false, "", "", "", ""}},
        [VK_F3] = {VK_F3, {1, SCANCODE_PREFIX_NONE, {0x3D}}, {true, false, "", "", "", ""}},
        [VK_F4] = {VK_F4, {1, SCANCODE_PREFIX_NONE, {0x3E}}, {true, false, "", "", "", ""}},
        [VK_F5] = {VK_F5, {1, SCANCODE_PREFIX_NONE, {0x3F}}, {true, false, "", "", "", ""}},
        [VK_F6] = {VK_F6, {1, SCANCODE_PREFIX_NONE, {0x40}}, {true, false, "", "", "", ""}},
        [VK_F7] = {VK_F7, {1, SCANCODE_PREFIX_NONE, {0x41}}, {true, false, "", "", "", ""}},
        [VK_F8] = {VK_F8, {1, SCANCODE_PREFIX_NONE, {0x42}}, {true, false, "", "", "", ""}},
        [VK_F9] = {VK_F9, {1, SCANCODE_PREFIX_NONE, {0x43}}, {true, false, "", "", "", ""}},
        [VK_F10] = {VK_F10, {1, SCANCODE_PREFIX_NONE, {0x44}}, {true, false, "", "", "", ""}},
        [VK_F11] = {VK_F11, {1, SCANCODE_PREFIX_NONE, {0x57}}, {true, false, "", "", "", ""}},
        [VK_F12] = {VK_F12, {1, SCANCODE_PREFIX_NONE, {0x58}}, {true, false, "", "", "", ""}},
        [VK_SNAPSHOT] = {VK_SNAPSHOT, {3, SCANCODE_PREFIX_E0, {0x2A, 0xE0, 0x37}}, {true, false, "", "", "", ""}},
        [VK_SNAPSHOT_CONTROL] = {VK_SNAPSHOT_CONTROL, {1, SCANCODE_PREFIX_E0, {0x37}}, {true, false, "", "", "", ""}},
        [VK_SNAPSHOT_SHIFT] = {VK_SNAPSHOT_SHIFT, {1, SCANCODE_PREFIX_E0, {0x37}}, {true, false, "", "", "", ""}},
        [VK_SNAPSHOT_ALT] = {VK_SNAPSHOT_ALT, {1, SCANCODE_PREFIX_NONE, {0x54}}, {true, false, "", "", "", ""}},
        [VK_SCROLL] = {VK_SCROLL, {1, SCANCODE_PREFIX_NONE, {0x46}}, {true, false, "", "", "", ""}},
        [VK_PAUSE] = {VK_PAUSE, {5, SCANCODE_PREFIX_E1, {0x1D, 0x45, 0xE1, 0x9D, 0xC5}}, {true, false, "", "", "", ""}},
        [VK_PAUSE_CONTROL] = {VK_PAUSE_CONTROL, {3, SCANCODE_PREFIX_E0, {0x46, 0xE0, 0xC6}}, {true, false, "", "", "", ""}},
        [VK_OEM_3] = {VK_OEM_3, {1, SCANCODE_PREFIX_NONE, {0x29}}, {false, false, "^", "°", "", ""}},
        [VK_1] = {VK_1, {1, SCANCODE_PREFIX_NONE, {0x02}}, {false, false, "1", "!", "", ""}},
        [VK_2] = {VK_2, {1, SCANCODE_PREFIX_NONE, {0x03}}, {false, false, "2", "\"", "²", ""}},
        [VK_3] = {VK_3, {1, SCANCODE_PREFIX_NONE, {0x04}}, {false, false, "3", "§", "³", ""}},
        [VK_4] = {VK_4, {1, SCANCODE_PREFIX_NONE, {0x05}}, {false, false, "4", "$", "", ""}},
        [VK_5] = {VK_5, {1, SCANCODE_PREFIX_NONE, {0x06}}, {false, false, "5", "%", "", ""}},
        [VK_6] = {VK_6, {1, SCANCODE_PREFIX_NONE, {0x07}}, {false, false, "6", "&", "", ""}},
        [VK_7] = {VK_7, {1, SCANCODE_PREFIX_NONE, {0x08}}, {false, false, "7", "/", "{", ""}},
        [VK_8] = {VK_8, {1, SCANCODE_PREFIX_NONE, {0x09}}, {false, false, "8", "(", "[", ""}},
        [VK_9] = {VK_9, {1, SCANCODE_PREFIX_NONE, {0x0A}}, {false, false, "9", ")", "]", ""}},
        [VK_0] = {VK_0, {1, SCANCODE_PREFIX_NONE, {0x0B}}, {false, false, "0", "=", "}", ""}},
        [VK_OEM_MINUS] = {VK_OEM_MINUS, {1, SCANCODE_PREFIX_NONE, {0x0C}}, {false, false, "ß", "?", "\\", ""}},
        [VK_OEM_PLUS] = {VK_OEM_PLUS, {1, SCANCODE_PREFIX_NONE, {0x0D}}, {false, false, "´", "`", "", ""}},
        [VK_BACK] = {VK_BACK, {1, SCANCODE_PREFIX_NONE, {0x0E}}, {true, false, "\b", "\b", "", ""}},
        [VK_TAB] = {VK_TAB, {1, SCANCODE_PREFIX_NONE, {0x0F}}, {true, false, "\t", "\t", "", ""}},
        [VK_Q] = {VK_Q, {1, SCANCODE_PREFIX_NONE, {0x10}}, {false, false, "q", "Q", "@", ""}},
        [VK_W] = {VK_W, {1, SCANCODE_PREFIX_NONE, {0x11}}, {false, false, "w", "W", "", ""}},
        [VK_E] = {VK_E, {1, SCANCODE_PREFIX_NONE, {0x12}}, {false, false, "e", "E", "€", ""}},
        [VK_R] = {VK_R, {1, SCANCODE_PREFIX_NONE, {0x13}}, {false, false, "r", "R", "", ""}},
        [VK_T] = {VK_T, {1, SCANCODE_PREFIX_NONE, {0x14}}, {false, false, "t", "T", "", ""}},
        [VK_Y] = {VK_Y, {1, SCANCODE_PREFIX_NONE, {0x15}}, {false, false, "z", "Z", "", ""}},
        [VK_U] = {VK_U, {1, SCANCODE_PREFIX_NONE, {0x16}}, {false, false, "u", "U", "", ""}},
        [VK_I] = {VK_I, {1, SCANCODE_PREFIX_NONE, {0x17}}, {false, false, "i", "I", "", ""}},
        [VK_O] = {VK_O, {1, SCANCODE_PREFIX_NONE, {0x18}}, {false, false, "o", "O", "", ""}},
        [VK_P] = {VK_P, {1, SCANCODE_PREFIX_NONE, {0x19}}, {false, false, "p", "P", "", ""}},
        [VK_OEM_4] = {VK_OEM_4, {1, SCANCODE_PREFIX_NONE, {0x1A}}, {false, false, "ü", "Ü", "", ""}},
        [VK_OEM_6] = {VK_OEM_6, {1, SCANCODE_PREFIX_NONE, {0x1B}}, {false, false, "+", "*", "~", ""}},
        [VK_RETURN] = {VK_RETURN, {1, SCANCODE_PREFIX_NONE, {0x1C}}, {true, false, "\n", "\n", "", ""}},
        [VK_CAPITAL] = {VK_CAPITAL, {1, SCANCODE_PREFIX_NONE, {0x3A}}, {true, false, "", "", "", ""}},
        [VK_A] = {VK_A, {1, SCANCODE_PREFIX_NONE, {0x1E}}, {false, false, "a", "A", "", ""}},
        [VK_S] = {VK_S, {1, SCANCODE_PREFIX_NONE, {0x1F}}, {false, false, "s", "S", "", ""}},
        [VK_D] = {VK_D, {1, SCANCODE_PREFIX_NONE, {0x20}}, {false, false, "d", "D", "", ""}},
        [VK_F] = {VK_F, {1, SCANCODE_PREFIX_NONE, {0x21}}, {false, false, "f", "F", "", ""}},
        [VK_G] = {VK_G, {1, SCANCODE_PREFIX_NONE, {0x22}}, {false, false, "g", "G", "", ""}},
        [VK_H] = {VK_H, {1, SCANCODE_PREFIX_NONE, {0x23}}, {false, false, "h", "H", "", ""}},
        [VK_J] = {VK_J, {1, SCANCODE_PREFIX_NONE, {0x24}}, {false, false, "j", "J", "", ""}},
        [VK_K] = {VK_K, {1, SCANCODE_PREFIX_NONE, {0x25}}, {false, false, "k", "K", "", ""}},
        [VK_L] = {VK_L, {1, SCANCODE_PREFIX_NONE, {0x26}}, {false, false, "l", "L", "", ""}},
        [VK_OEM_1] = {VK_OEM_1, {1, SCANCODE_PREFIX_NONE, {0x27}}, {false, false, "ö", "Ö", "", ""}},
        [VK_OEM_7] = {VK_OEM_7, {1, SCANCODE_PREFIX_NONE, {0x28}}, {false, false, "ä", "Ä", "", ""}},
        [VK_OEM_5] = {VK_OEM_5, {1, SCANCODE_PREFIX_NONE, {0x2B}}, {false, false, "#", "'", "", ""}},
        [VK_LSHIFT] = {VK_LSHIFT, {1, SCANCODE_PREFIX_NONE, {0x2A}}, {true, false, "", "", "", ""}},
        [VK_OEM_102] = {VK_OEM_102, {1, SCANCODE_PREFIX_NONE, {0x56}}, {false, false, "<", ">", "|", ""}},
        [VK_Z] = {VK_Z, {1, SCANCODE_PREFIX_NONE, {0x2C}}, {false, false, "y", "Y", "", ""}},
        [VK_X] = {VK_X, {1, SCANCODE_PREFIX_NONE, {0x2D}}, {false, false, "x", "X", "", ""}},
        [VK_C] = {VK_C, {1, SCANCODE_PREFIX_NONE, {0x2E}}, {false, false, "c", "C", "", ""}},
        [VK_V] = {VK_V, {1, SCANCODE_PREFIX_NONE, {0x2F}}, {false, false, "v", "V", "", ""}},
        [VK_B] = {VK_B, {1, SCANCODE_PREFIX_NONE, {0x30}}, {false, false, "b", "B", "", ""}},
        [VK_N] = {VK_N, {1, SCANCODE_PREFIX_NONE, {0x31}}, {false, false, "n", "N", "", ""}},
        [VK_M] = {VK_M, {1, SCANCODE_PREFIX_NONE, {0x32}}, {false, false, "m", "M", "µ", ""}},
        [VK_OEM_COMMA] = {VK_OEM_COMMA, {1, SCANCODE_PREFIX_NONE, {0x33}}, {false, false, ",", ";", "", ""}},
        [VK_OEM_PERIOD] = {VK_OEM_PERIOD, {1, SCANCODE_PREFIX_NONE, {0x34}}, {false, false, ".", ":", "", ""}},
        [VK_OEM_2] = {VK_OEM_2, {1, SCANCODE_PREFIX_NONE, {0x35}}, {false, false, "-", "_", "", ""}},
        [VK_RSHIFT] = {VK_RSHIFT, {1, SCANCODE_PREFIX_NONE, {0x36}}, {true, false, "", "", "", ""}},
        [VK_LCONTROL] = {VK_LCONTROL, {1, SCANCODE_PREFIX_NONE, {0x1D}}, {true, false, "", "", "", ""}},
        [VK_LWIN] = {VK_LWIN, {1, SCANCODE_PREFIX_E0, {0x5B}}, {true, false, "", "", "", ""}},
        [VK_LMENU] = {VK_LMENU, {1, SCANCODE_PREFIX_NONE, {0x38}}, {true, false, "", "", "", ""}},
        [VK_SPACE] = {VK_SPACE, {1, SCANCODE_PREFIX_NONE, {0x39}}, {false, false, " ", " ", "", ""}},
        [VK_RMENU] = {VK_RMENU, {1, SCANCODE_PREFIX_E0, {0x38}}, {true, false, "", "", "", ""}},
        [VK_RWIN] = {VK_RWIN, {1, SCANCODE_PREFIX_E0, {0x5C}}, {true, false, "", "", "", ""}},
        [VK_APPS] = {VK_APPS, {1, SCANCODE_PREFIX_E0, {0x5D}}, {true, false, "", "", "", ""}},
        [VK_RCONTROL] = {VK_RCONTROL, {1, SCANCODE_PREFIX_E0, {0x1D}}, {true, false, "", "", "", ""}},
        [VK_INSERT] = {VK_INSERT, {1, SCANCODE_PREFIX_E0, {0x52}}, {true, false, "", "", "", ""}},
        [VK_HOME] = {VK_HOME, {1, SCANCODE_PREFIX_E0, {0x47}}, {true, false, "", "", "", ""}},
        [VK_PRIOR] = {VK_PRIOR, {1, SCANCODE_PREFIX_E0, {0x49}}, {true, false, "", "", "", ""}},
        [VK_DELETE] = {VK_DELETE, {1, SCANCODE_PREFIX_E0, {0x53}}, {true, false, "", "", "", ""}},
        [VK_END] = {VK_END, {1, SCANCODE_PREFIX_E0, {0x4F}}, {true, false, "", "", "", ""}},
        [VK_NEXT] = {VK_NEXT, {1, SCANCODE_PREFIX_E0, {0x51}}, {true, false, "", "", "", ""}},
        [VK_LEFT] = {VK_LEFT, {1, SCANCODE_PREFIX_E0, {0x4B}}, {true, false, "", "", "", ""}},
        [VK_UP] = {VK_UP, {1, SCANCODE_PREFIX_E0, {0x48}}, {true, false, "", "", "", ""}},
        [VK_RIGHT] = {VK_RIGHT, {1, SCANCODE_PREFIX_E0, {0x4D}}, {true, false, "", "", "", ""}},
        [VK_DOWN] = {VK_DOWN, {1, SCANCODE_PREFIX_E0, {0x50}}, {true, false, "", "", "", ""}},
        [VK_NUMLOCK] = {VK_NUMLOCK, {1, SCANCODE_PREFIX_NONE, {0x45}}, {true, false, "", "", "", ""}},
        [VK_DIVIDE] = {VK_DIVIDE, {1, SCANCODE_PREFIX_E0, {0x35}}, {false, false, "/", "/", "", "/"}},
        [VK_MULTIPLY] = {VK_MULTIPLY, {1, SCANCODE_PREFIX_NONE, {0x37}}, {false, false, "*", "*", "", "*"}},
        [VK_SUBTRACT] = {VK_SUBTRACT, {1, SCANCODE_PREFIX_NONE, {0x4A}}, {false, false, "-", "-", "", "-"}},
        [VK_ADD] = {VK_ADD, {1, SCANCODE_PREFIX_NONE, {0x4E}}, {false, false, "+", "+", "", "+"}},
        [VK_NUM_HOME] = {VK_NUM_HOME, {1, SCANCODE_PREFIX_NONE, {0x47}}, {true, true, "", "", "", "7"}},
        [VK_NUM_UP] = {VK_NUM_UP, {1, SCANCODE_PREFIX_NONE, {0x48}}, {true, true, "", "", "", "8"}},
        [VK_NUM_PRIOR] = {VK_NUM_PRIOR, {1, SCANCODE_PREFIX_NONE, {0x49}}, {true, true, "", "", "", "9"}},
        [VK_NUM_LEFT] = {VK_NUM_LEFT, {1, SCANCODE_PREFIX_NONE, {0x4B}}, {true, true, "", "", "", "4"}},
        [VK_NUM_CLEAR] = {VK_NUM_CLEAR, {1, SCANCODE_PREFIX_NONE, {0x4C}}, {true, true, "", "", "", "5"}},
        [VK_NUM_RIGHT] = {VK_NUM_RIGHT, {1, SCANCODE_PREFIX_NONE, {0x4D}}, {true, true, "", "", "", "6"}},
        [VK_NUM_END] = {VK_NUM_END, {1, SCANCODE_PREFIX_NONE, {0x4F}}, {true, true, "", "", "", "1"}},
        [VK_NUM_DOWN] = {VK_NUM_DOWN, {1, SCANCODE_PREFIX_NONE, {0x50}}, {true, true, "", "", "", "2"}},
        [VK_NUM_NEXT] = {VK_NUM_NEXT, {1, SCANCODE_PREFIX_NONE, {0x51}}, {true, true, "", "", "", "3"}},
        [VK_NUM_INSERT] = {VK_NUM_INSERT, {1, SCANCODE_PREFIX_NONE, {0x52}}, {true, true, "", "", "", "0"}},
        [VK_NUM_DELETE] = {VK_NUM_DELETE, {1, SCANCODE_PREFIX_NONE, {0x53}}, {true, true, "", "", "", ","}},
        [VK_NUM_RETURN] = {VK_NUM_RETURN, {1, SCANCODE_PREFIX_E0, {0x1C}}, {true, false, "\n", "\n", "", "\n"}},
    }
};

/**
 * @brief Formats a scancode sequence into a hex string.
 * @param buf The destination buffer.
 * @param seq The scancode sequence to format.
 */
static void format_scancodes(char *buf, scancode_sequence_t *seq) {
    int pos = 0;
    for (int b = 0; b < seq->length && b < 5; b++) {
        uint8_t val = seq->bytes[b];
        const char *hex = "0123456789ABCDEF";
        buf[pos++] = hex[(val >> 4) & 0x0F];
        buf[pos++] = hex[val & 0x0F];
        buf[pos++] = ' ';
    }
    buf[pos] = '\0';
}

/**
 * @brief Returns a printable representation of a key mapping string.
 * @param str The mapping string.
 * @return A pointer to a printable string.
 */
static const char* safe_map(const char* str) {
    if (!str || str[0] == '\0') return "-";
    
    switch (str[0]) {
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        case '\b': return "\\b";
        case 127:  return "DEL";
        default:   return str;
    }
}

/**
 * @brief Dumps the keyboard layout to the serial console for debugging.
 * @param layout Pointer to the keyboard layout to dump.
 */
void dump_layout(keyboard_layout_t *layout) {
    serial_printf("\n--- Keyboard Layout: %s ---\n", layout->lang);
    
    serial_printf("%-3s | %-6s | %-15s | %-4s | %-4s | %-6s | %-6s | %-6s | %-6s\n", 
                  "ID", "Prfx", "Scancodes", "Spcl", "NumK", "Norm", "Shft", "AltG", "Num");
    serial_printf("--------------------------------------------------------------------------------\n");

    for (int i = 0; i < VK_COUNT; i++) {
        key_t *k = &layout->keys[i];
        if (k->scancode_sequence.length == 0) continue;

        char sc_buf[32]; 
        format_scancodes(sc_buf, &k->scancode_sequence);

        serial_printf("%-3d | %-6x | %-15s |  %c   |  %c   | %-6s | %-6s | %-6s | %-6s\n", 
            i, 
            k->scancode_sequence.prefix,
            sc_buf,
            k->mapping.is_special   ? 'Y' : 'N',
            k->mapping.is_num_key   ? 'Y' : 'N',
            safe_map(k->mapping.normal),
            safe_map(k->mapping.shift),
            safe_map(k->mapping.alt_gr),
            safe_map(k->mapping.num));
    }
    serial_printf("--------------------------------------------------------------------------------\n");
}