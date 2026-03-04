/**
 * @file keyboard.c
 * @author friedrichOsDev
 */

#include <keyboard.h>
#include <timer.h>
#include <i8042.h>
#include <print.h>
#include <kernel.h>
#include <serial.h>
#include <string.h>

static const keyboard_layout_t* current_layout = NULL;
static kbd_scancode_buffer_t scancode_ring = { .head = 0, .tail = 0 };
static kbd_data_buffer_t data_ring = { .head = 0, .tail = 0 };
static keyboard_state_t kbd_state = {
    .shift = false,
    .ctrl = false,
    .alt = false,
    .alt_gr = false,
    .caps_lock = false,
    .num_lock = false,
    .scroll_lock = false
};
static virtual_key_t last_dead_key = VK_NONE;
static keyboard_state_t last_dead_key_state = {0};
static vk_action_t vk_to_function_map[VK_COUNT] = {0};


/**
 * @brief Initializes the keyboard driver with a specific layout.
 * @param layout Pointer to the keyboard layout to use.
 */
void keyboard_init(const keyboard_layout_t* layout) {
    current_layout = layout;

    restore_default_map();

    irq_install_handler(1, keyboard_callback);

    event_t kbd_update_event = {
        .event_id = 0,
        .handler = keyboard_update,
        .interval = 1,
        .target_tick = timer_get_ticks() + 1,
        .repeat = true,
        .active = true
    };
    uint32_t kbd_update_event_id = timer_add_event(kbd_update_event);
    (void)kbd_update_event_id;
    init_state = INIT_KBD;
}

/**
 * @brief IRQ 1 handler. Reads raw scancodes from the i8042 controller.
 * @param regs CPU registers at the time of interrupt.
 */
void keyboard_callback(struct registers *regs) {
    (void)regs;
    uint8_t status = i8042_read_status();
    if (status & I8042_STATUS_OUTPUT_BUFFER_FULL) {
        uint8_t scancode = i8042_read_data();
        buffer_push(scancode_ring.buffer, &scancode_ring.head, scancode_ring.tail, KBD_BUFFER_SIZE, scancode);
    }
}

/**
 * @brief Retrieves the next Unicode character from the data buffer.
 * @return The Unicode character, or 0 if the buffer is empty.
 */
uint32_t keyboard_get_unicode(void) {
    return buffer_pop(data_ring.buffer, &data_ring.head, &data_ring.tail, KBD_BUFFER_SIZE);
}

/**
 * @brief Maps a custom action function to a virtual key.
 * @param vk The virtual key to map.
 * @param action The function to execute when the key is pressed.
 */
void keyboard_map_function_to_vk(virtual_key_t vk, vk_action_t action) {
    if (vk < VK_COUNT) {
        vk_to_function_map[vk] = action;
    }
}

/**
 * @brief Restores the entire default function map for all keys.
 */
void restore_default_map(void) {
    keyboard_map_function_to_vk(VK_ESCAPE, action_escape);
    keyboard_map_function_to_vk(VK_F1, action_f1);
    keyboard_map_function_to_vk(VK_F2, action_f2);
    keyboard_map_function_to_vk(VK_F3, action_f3);
    keyboard_map_function_to_vk(VK_F4, action_f4);
    keyboard_map_function_to_vk(VK_F5, action_f5);
    keyboard_map_function_to_vk(VK_F6, action_f6);
    keyboard_map_function_to_vk(VK_F7, action_f7);
    keyboard_map_function_to_vk(VK_F8, action_f8);
    keyboard_map_function_to_vk(VK_F9, action_f9);
    keyboard_map_function_to_vk(VK_F10, action_f10);
    keyboard_map_function_to_vk(VK_F11, action_f11);
    keyboard_map_function_to_vk(VK_F12, action_f12);

    keyboard_map_function_to_vk(VK_SNAPSHOT, action_snapshot);
    keyboard_map_function_to_vk(VK_SNAPSHOT_CONTROL, action_snapshot_ctrl);
    keyboard_map_function_to_vk(VK_SNAPSHOT_SHIFT, action_snapshot_shift);
    keyboard_map_function_to_vk(VK_SNAPSHOT_ALT, action_snapshot_alt);
    keyboard_map_function_to_vk(VK_SCROLL, action_scroll);
    keyboard_map_function_to_vk(VK_PAUSE, action_pause);
    keyboard_map_function_to_vk(VK_PAUSE_CONTROL, action_pause_control);
    
    keyboard_map_function_to_vk(VK_BACK, action_backspace);
    keyboard_map_function_to_vk(VK_TAB, action_tab);
    keyboard_map_function_to_vk(VK_RETURN, action_return);
    keyboard_map_function_to_vk(VK_CAPITAL, action_capital);
    
    keyboard_map_function_to_vk(VK_LSHIFT, action_lshift);
    keyboard_map_function_to_vk(VK_RSHIFT, action_rshift);
    keyboard_map_function_to_vk(VK_LCONTROL, action_lcontrol);
    keyboard_map_function_to_vk(VK_RCONTROL, action_rcontrol);
    keyboard_map_function_to_vk(VK_LWIN, action_lwin);
    keyboard_map_function_to_vk(VK_RWIN, action_rwin);
    keyboard_map_function_to_vk(VK_LMENU, action_lmenu);
    keyboard_map_function_to_vk(VK_RMENU, action_rmenu);
    keyboard_map_function_to_vk(VK_APPS, action_apps);
    
    keyboard_map_function_to_vk(VK_INSERT, action_insert);
    keyboard_map_function_to_vk(VK_DELETE, action_delete);
    keyboard_map_function_to_vk(VK_HOME, action_home);
    keyboard_map_function_to_vk(VK_END, action_end);
    keyboard_map_function_to_vk(VK_PRIOR, action_prior);
    keyboard_map_function_to_vk(VK_NEXT, action_next);
    
    keyboard_map_function_to_vk(VK_LEFT, action_left);
    keyboard_map_function_to_vk(VK_RIGHT, action_right);
    keyboard_map_function_to_vk(VK_UP, action_up);
    keyboard_map_function_to_vk(VK_DOWN, action_down);
    
    keyboard_map_function_to_vk(VK_NUMLOCK, action_numlock);
    keyboard_map_function_to_vk(VK_NUM_HOME, action_home);
    keyboard_map_function_to_vk(VK_NUM_UP, action_up);
    keyboard_map_function_to_vk(VK_NUM_PRIOR, action_prior);
    keyboard_map_function_to_vk(VK_NUM_LEFT, action_left);
    keyboard_map_function_to_vk(VK_NUM_CLEAR, action_clear);
    keyboard_map_function_to_vk(VK_NUM_RIGHT, action_right);
    keyboard_map_function_to_vk(VK_NUM_END, action_end);
    keyboard_map_function_to_vk(VK_NUM_DOWN, action_down);
    keyboard_map_function_to_vk(VK_NUM_NEXT, action_next);
    keyboard_map_function_to_vk(VK_NUM_INSERT, action_insert);
    keyboard_map_function_to_vk(VK_NUM_DELETE, action_delete);
    keyboard_map_function_to_vk(VK_NUM_RETURN, action_return);
}

/**
 * @brief Restores the default action for a single virtual key.
 * @param vk The virtual key to restore.
 */
void restore_default_mapping(virtual_key_t vk) {
    if (vk >= VK_COUNT) return;

    switch (vk) {
        case VK_ESCAPE: vk_to_function_map[vk] = action_escape; break;
        case VK_F1: vk_to_function_map[vk] = action_f1; break;
        case VK_F2: vk_to_function_map[vk] = action_f2; break;
        case VK_F3: vk_to_function_map[vk] = action_f3; break;
        case VK_F4: vk_to_function_map[vk] = action_f4; break;
        case VK_F5: vk_to_function_map[vk] = action_f5; break;
        case VK_F6: vk_to_function_map[vk] = action_f6; break;
        case VK_F7: vk_to_function_map[vk] = action_f7; break;
        case VK_F8: vk_to_function_map[vk] = action_f8; break;
        case VK_F9: vk_to_function_map[vk] = action_f9; break;
        case VK_F10: vk_to_function_map[vk] = action_f10; break;
        case VK_F11: vk_to_function_map[vk] = action_f11; break;
        case VK_F12: vk_to_function_map[vk] = action_f12; break;
        case VK_SNAPSHOT: vk_to_function_map[vk] = action_snapshot; break;
        case VK_SNAPSHOT_CONTROL: vk_to_function_map[vk] = action_snapshot_ctrl; break;
        case VK_SNAPSHOT_SHIFT: vk_to_function_map[vk] = action_snapshot_shift; break;
        case VK_SNAPSHOT_ALT: vk_to_function_map[vk] = action_snapshot_alt; break;
        case VK_SCROLL: vk_to_function_map[vk] = action_scroll; break;
        case VK_PAUSE: vk_to_function_map[vk] = action_pause; break;
        case VK_PAUSE_CONTROL: vk_to_function_map[vk] = action_pause_control; break;
        case VK_BACK: vk_to_function_map[vk] = action_backspace; break;
        case VK_TAB: vk_to_function_map[vk] = action_tab; break;
        case VK_RETURN: vk_to_function_map[vk] = action_return; break;
        case VK_CAPITAL: vk_to_function_map[vk] = action_capital; break;
        case VK_LSHIFT: vk_to_function_map[vk] = action_lshift; break;
        case VK_RSHIFT: vk_to_function_map[vk] = action_rshift; break;
        case VK_LCONTROL: vk_to_function_map[vk] = action_lcontrol; break;
        case VK_RCONTROL: vk_to_function_map[vk] = action_rcontrol; break;
        case VK_LWIN: vk_to_function_map[vk] = action_lwin; break;
        case VK_RWIN: vk_to_function_map[vk] = action_rwin; break;
        case VK_LMENU: vk_to_function_map[vk] = action_lmenu; break;
        case VK_RMENU: vk_to_function_map[vk] = action_rmenu; break;
        case VK_APPS: vk_to_function_map[vk] = action_apps; break;
        case VK_INSERT: vk_to_function_map[vk] = action_insert; break;
        case VK_DELETE: vk_to_function_map[vk] = action_delete; break;
        case VK_HOME: vk_to_function_map[vk] = action_home; break;
        case VK_END: vk_to_function_map[vk] = action_end; break;
        case VK_PRIOR: vk_to_function_map[vk] = action_prior; break;
        case VK_NEXT: vk_to_function_map[vk] = action_next; break;
        case VK_LEFT: vk_to_function_map[vk] = action_left; break;
        case VK_RIGHT: vk_to_function_map[vk] = action_right; break;
        case VK_UP: vk_to_function_map[vk] = action_up; break;
        case VK_DOWN: vk_to_function_map[vk] = action_down; break;
        case VK_NUMLOCK: vk_to_function_map[vk] = action_numlock; break;
        case VK_NUM_HOME: vk_to_function_map[vk] = action_home; break;
        case VK_NUM_UP: vk_to_function_map[vk] = action_up; break;
        case VK_NUM_PRIOR: vk_to_function_map[vk] = action_prior; break;
        case VK_NUM_LEFT: vk_to_function_map[vk] = action_left; break;
        case VK_NUM_CLEAR: vk_to_function_map[vk] = action_clear; break;
        case VK_NUM_RIGHT: vk_to_function_map[vk] = action_right; break;
        case VK_NUM_END: vk_to_function_map[vk] = action_end; break;
        case VK_NUM_DOWN: vk_to_function_map[vk] = action_down; break;
        case VK_NUM_NEXT: vk_to_function_map[vk] = action_next; break;
        case VK_NUM_INSERT: vk_to_function_map[vk] = action_insert; break;
        case VK_NUM_DELETE: vk_to_function_map[vk] = action_delete; break;
        case VK_NUM_RETURN: vk_to_function_map[vk] = action_return; break;
        default: vk_to_function_map[vk] = NULL; break;
    }
}


/**
 * @brief Pushes a byte into a ring buffer.
 * @param buffer Pointer to the buffer array.
 * @param head Pointer to the head index.
 * @param tail The current tail index.
 * @param buffer_size Size of the buffer.
 * @param value The byte to push.
 */
void buffer_push(void* buffer, uint16_t* head, uint16_t tail, size_t buffer_size, uint8_t value) {
    size_t next = (*head + 1) % buffer_size;
    if (next != tail) {
        ((uint8_t*)buffer)[*head] = value;
        *head = next;
    }
}

/**
 * @brief Pops a value from a ring buffer.
 * @param buffer Pointer to the buffer array.
 * @param head Pointer to the head index.
 * @param tail Pointer to the tail index.
 * @param buffer_size Size of the buffer.
 * @return The popped value, or 0 if empty.
 */
uint32_t buffer_pop(void* buffer, uint16_t* head, uint16_t* tail, size_t buffer_size) {
    if (*tail == *head) {
        return 0; // Buffer is empty
    }
    uint32_t value = ((uint8_t*)buffer)[*tail];
    *tail = (*tail + 1) % buffer_size;
    return value;
}

/**
 * @brief Advances the tail of the scancode ring buffer.
 * @param count Number of bytes to remove.
 * @return true.
 */
bool buffer_remove_start(uint16_t count) {
    scancode_ring.tail = (scancode_ring.tail + count) % KBD_BUFFER_SIZE;
    return true;
}


bool action_escape(bool release) { if (release) return true; serial_printf("Action: ESCAPE\n"); return true; }
bool action_f1(bool release) { if (release) return true; serial_printf("Action: F1\n"); return true; }
bool action_f2(bool release) { if (release) return true; serial_printf("Action: F2\n"); return true; }
bool action_f3(bool release) { if (release) return true; serial_printf("Action: F3\n"); return true; }
bool action_f4(bool release) { if (release) return true; serial_printf("Action: F4\n"); return true; }
bool action_f5(bool release) { if (release) return true; serial_printf("Action: F5\n"); return true; }
bool action_f6(bool release) { if (release) return true; serial_printf("Action: F6\n"); return true; }
bool action_f7(bool release) { if (release) return true; serial_printf("Action: F7\n"); return true; }
bool action_f8(bool release) { if (release) return true; serial_printf("Action: F8\n"); return true; }
bool action_f9(bool release) { if (release) return true; serial_printf("Action: F9\n"); return true; }
bool action_f10(bool release) { if (release) return true; serial_printf("Action: F10\n"); return true; }
bool action_f11(bool release) { if (release) return true; serial_printf("Action: F11\n"); return true; }
bool action_f12(bool release) { if (release) return true; serial_printf("Action: F12\n"); return true; }
bool action_snapshot(bool release) { if (release) return true; serial_printf("Action: PRINT SCREEN\n"); return true; }
bool action_snapshot_ctrl(bool release) { if (release) return true; serial_printf("Action: CTRL + PRINT SCREEN\n"); return true; }
bool action_snapshot_shift(bool release) { if (release) return true; serial_printf("Action: SHIFT + PRINT SCREEN\n"); return true; }
bool action_snapshot_alt(bool release) { if (release) return true; serial_printf("Action: ALT + PRINT SCREEN\n"); return true; }
bool action_scroll(bool release) { if (release) return true; serial_printf("Action: SCROLL LOCK\n"); return true; }
bool action_pause(bool release) { if (release) return true; serial_printf("Action: PAUSE\n"); return true; }
bool action_pause_control(bool release) { if (release) return true; serial_printf("Action: CTRL + PAUSE\n"); return true; }

bool action_backspace(bool release) { if (release) return true; serial_printf("Action: BACKSPACE\n"); buffer_push(data_ring.buffer, &data_ring.head, data_ring.tail, KBD_BUFFER_SIZE, 0x0008); return true; }
bool action_tab(bool release) { if (release) return true; serial_printf("Action: TAB\n"); buffer_push(data_ring.buffer, &data_ring.head, data_ring.tail, KBD_BUFFER_SIZE, 0x0009); return true; }
bool action_return(bool release) { if (release) return true; serial_printf("Action: RETURN\n"); buffer_push(data_ring.buffer, &data_ring.head, data_ring.tail, KBD_BUFFER_SIZE, 0x000A); return true; }
bool action_capital(bool release) { if (release) return true; serial_printf("Action: CAPS LOCK\n"); kbd_state.caps_lock = !kbd_state.caps_lock; return true; }
bool action_lshift(bool release) { if (!release) serial_printf("Action: LEFT SHIFT\n"); kbd_state.shift = !release; return true; }
bool action_rshift(bool release) { if (!release) serial_printf("Action: RIGHT SHIFT\n"); kbd_state.shift = !release; return true; }
bool action_lcontrol(bool release) { if (!release) serial_printf("Action: LEFT CTRL\n"); kbd_state.ctrl = !release; return true; }
bool action_rcontrol(bool release) { if (!release) serial_printf("Action: RIGHT CTRL\n"); kbd_state.ctrl = !release; return true; }
bool action_lwin(bool release) { if (release) return true; serial_printf("Action: LEFT WIN\n"); return true; }
bool action_rwin(bool release) { if (release) return true; serial_printf("Action: RIGHT WIN\n"); return true; }
bool action_lmenu(bool release) { if (!release) serial_printf("Action: LEFT ALT\n"); kbd_state.alt = !release; return true; }
bool action_rmenu(bool release) { if (!release) serial_printf("Action: RIGHT ALT / ALT GR\n"); kbd_state.alt_gr = !release; return true; }
bool action_apps(bool release) { if (release) return true; serial_printf("Action: CONTEXT MENU (APPS)\n"); return true; }

bool action_insert(bool release) { if (release) return true; serial_printf("Action: INSERT\n"); return true; }
bool action_home(bool release) { if (release) return true; serial_printf("Action: HOME\n"); return true; }
bool action_prior(bool release) { if (release) return true; serial_printf("Action: PAGE UP\n"); return true; }
bool action_delete(bool release) { if (release) return true; serial_printf("Action: DELETE\n"); return true; }
bool action_end(bool release) { if (release) return true; serial_printf("Action: END\n"); return true; }
bool action_next(bool release) { if (release) return true; serial_printf("Action: PAGE DOWN\n"); return true; }
bool action_left(bool release) { if (release) return true; serial_printf("Action: ARROW LEFT\n"); return true; }
bool action_up(bool release) { if (release) return true; serial_printf("Action: ARROW UP\n"); return true; }
bool action_right(bool release) { if (release) return true; serial_printf("Action: ARROW RIGHT\n"); return true; }
bool action_down(bool release) { if (release) return true; serial_printf("Action: ARROW DOWN\n"); return true; }

bool action_numlock(bool release) { if (release) return true; serial_printf("Action: NUMLOCK\n"); kbd_state.num_lock = !kbd_state.num_lock; return true; }
bool action_clear(bool release) { if (release) return true; serial_printf("Action: NUMPAD CLEAR (5)\n"); return true; }


/**
 * @brief Fallback handler for special keys that don't have a specific action.
 * @param vk The virtual key.
 * @param release True if the key was released.
 */
void keyboard_handle_special(virtual_key_t vk, bool release) {
    if (vk >= VK_COUNT) return;
    if (!release) serial_printf("Special key pressed but no action: %d\n", vk);
}

/**
 * @brief Handles keys belonging to the numeric keypad.
 * @param vk The virtual key.
 * @param release True if the key was released.
 */
void keyboard_handle_num_key(virtual_key_t vk, bool release) {
    if (release) return;

    if (kbd_state.num_lock) {
        uint32_t unicode = get_unicode_for_state(vk, kbd_state);
        if (unicode != 0) {
            buffer_push(data_ring.buffer, &data_ring.head, data_ring.tail, KBD_BUFFER_SIZE, unicode);
        }
    } else {
        if (vk_to_function_map[vk] != NULL) {
            vk_to_function_map[vk](release);
        }
    }
}

/**
 * @brief Checks if a virtual key is an alphabetic character.
 * @param vk The virtual key.
 * @return True if alphabetic.
 */
bool vk_is_alpha(virtual_key_t vk) {
    return vk_is_alpha_map[vk];
}

/**
 * @brief Checks if a virtual key is a dead key in the current layout.
 * @param vk The virtual key.
 * @return True if it is a dead key.
 */
bool vk_is_dead(virtual_key_t vk) {
    return current_layout->vk_to_unicode[vk].flags.is_dead_key;
}

/**
 * @brief Checks if a virtual key is marked as special.
 * @param vk The virtual key.
 * @return True if special.
 */
bool vk_is_special(virtual_key_t vk) {
    return current_layout->vk_to_unicode[vk].flags.is_special;
}

/**
 * @brief Checks if a virtual key is a numpad key.
 * @param vk The virtual key.
 * @return True if it is a numpad key.
 */
bool vk_is_num_key(virtual_key_t vk) {
    return current_layout->vk_to_unicode[vk].flags.is_num_key;
}

/**
 * @brief Looks up a combined character from a dead key and a base character.
 * @param dead_char The Unicode character of the dead key.
 * @param base_char The Unicode character of the base key.
 * @return The combined Unicode character, or 0 if no combination exists.
 */
uint32_t keyboard_get_combined_char(uint32_t dead_char, uint32_t base_char) {
    for (int i = 0; dead_key_table[i].dead_char != 0; i++) {
        if (dead_key_table[i].dead_char == dead_char && dead_key_table[i].base_char == base_char) {
            return dead_key_table[i].combined;
        }
    }
    return 0;
}

/**
 * @brief Merges a pending dead key with a new key press.
 * @param dead_vk The virtual key of the dead key.
 * @param vk The virtual key of the base character.
 */
void keyboard_merge_dead_key(virtual_key_t dead_vk, virtual_key_t vk) {
    uint32_t dead_char = get_unicode_for_state(dead_vk, last_dead_key_state);
    uint32_t base_char = get_unicode_for_state(vk, kbd_state);
    uint32_t combined = keyboard_get_combined_char(dead_char, base_char);
    if (combined) {
        buffer_push(data_ring.buffer, &data_ring.head, data_ring.tail, KBD_BUFFER_SIZE, combined);
    } else {
        buffer_push(data_ring.buffer, &data_ring.head, data_ring.tail, KBD_BUFFER_SIZE, dead_char);
        buffer_push(data_ring.buffer, &data_ring.head, data_ring.tail, KBD_BUFFER_SIZE, base_char);
    }
}

/**
 * @brief Translates a virtual key to Unicode based on the provided modifier state.
 * @param vk The virtual key.
 * @param state The modifier state to use for translation.
 * @return The resulting Unicode character.
 */
uint32_t get_unicode_for_state(virtual_key_t vk, keyboard_state_t state) {
    vk_to_unicode_t vk_uni = current_layout->vk_to_unicode[vk];
    
    if (vk_is_num_key(vk) && state.num_lock) return vk_uni.num;

    if (state.alt_gr) return vk_uni.alt_gr;

    bool effective_shift = state.shift;
    if (vk_is_alpha(vk)) effective_shift ^= state.caps_lock;

    if (effective_shift) return vk_uni.shift;
    if (state.ctrl) return vk_uni.ctrl;

    return vk_uni.normal;
}

/**
 * @brief High-level handler for a virtual key event.
 * @param vk The virtual key.
 * @param release True if the key was released.
 */
void keyboard_handle_key(virtual_key_t vk, bool release) {
    if (vk == VK_NONE) return;

    if (vk_is_num_key(vk)) {
        keyboard_handle_num_key(vk, release);
        return;
    }

    if (vk_to_function_map[vk] != NULL) {
        if (vk_to_function_map[vk](release)) return;
    }

    if (release) return;

    if (vk_is_dead(vk)) {
        if (last_dead_key != VK_NONE) {
            keyboard_merge_dead_key(last_dead_key, vk);
            last_dead_key = VK_NONE;
            last_dead_key_state = (keyboard_state_t){0};
        } else {
            last_dead_key = vk;
            last_dead_key_state = kbd_state;
        }
        return;
    }

    uint32_t unicode = get_unicode_for_state(vk, kbd_state);
    if (unicode == 0) return;

    if (last_dead_key != VK_NONE) {
        keyboard_merge_dead_key(last_dead_key, vk);
        last_dead_key = VK_NONE;
        last_dead_key_state = (keyboard_state_t){0};
        return;
    }

    buffer_push(data_ring.buffer, &data_ring.head, data_ring.tail, KBD_BUFFER_SIZE, unicode);
}


/**
 * @brief Attempts to match a scancode sequence against known complex sequences.
 * @param seq The sequence to check.
 * @param out_vk Pointer to store the resulting virtual key.
 * @return True if a match was found.
 */
bool try_parse_sequence(const scancode_sequence_t* seq, virtual_key_t* out_vk) {
    const scancode_sequence_t pause_target = scancode_to_vk_map.sc_to_vk[VK_PAUSE].sequence;
    const scancode_sequence_t prtsc_target = scancode_to_vk_map.sc_to_vk[VK_SNAPSHOT].sequence;
    const scancode_sequence_t pause_ctrl_target = scancode_to_vk_map.sc_to_vk[VK_PAUSE_CONTROL].sequence;

    if (seq->length == pause_target.length && seq->prefix == pause_target.prefix) {
        if (memcmp(seq->bytes, pause_target.bytes, seq->length) == 0) {
            *out_vk = VK_PAUSE;
            return true;
        }
    }

    if (seq->length == prtsc_target.length && seq->prefix == prtsc_target.prefix) {
        if (memcmp(seq->bytes, prtsc_target.bytes, seq->length) == 0) {
            *out_vk = VK_SNAPSHOT;
            return true;
        }
    }

    if (seq->length == pause_ctrl_target.length && seq->prefix == pause_ctrl_target.prefix) {
        if (memcmp(seq->bytes, pause_ctrl_target.bytes, seq->length) == 0) {
            *out_vk = VK_PAUSE_CONTROL;
            return true;
        }
    }

    return false;
}

/**
 * @brief Calculates the number of bytes available in the scancode ring buffer.
 * @return Number of bytes.
 */
static inline uint16_t kbd_available(void) {
    if (scancode_ring.head >= scancode_ring.tail) {
        return scancode_ring.head - scancode_ring.tail;
    } else {
        return KBD_BUFFER_SIZE - (scancode_ring.tail - scancode_ring.head);
    }
}

/**
 * @brief Peeks at a byte in the scancode ring buffer without removing it.
 * @param offset Offset from the tail.
 * @return The byte at the offset.
 */
static inline uint8_t kbd_peek(uint16_t offset) {
    return scancode_ring.buffer[(scancode_ring.tail + offset) % KBD_BUFFER_SIZE];
}

/**
 * @brief Handles the complex E1 scancode sequence (Pause key).
 */
void handle_e1_sequence(void) {
    scancode_sequence_t seq = {0};
    seq.prefix = SCANCODE_PREFIX_E1;
    seq.length = 5;

    for (size_t i = 0; i < 5; i++) {
        seq.bytes[i] = kbd_peek(i + 1);
    }

    virtual_key_t vk = VK_NONE;
    if (try_parse_sequence(&seq, &vk)) {
        keyboard_handle_key(vk, false);
    }
}

/**
 * @brief Handles scancodes starting with the E0 prefix.
 */
void handle_e0_prefix(void) {
    uint8_t b2 = kbd_peek(1);
    uint8_t e0_code = b2;
    bool release = false;
    if (b2 & 0x80) {
        e0_code = b2 & 0x7F;
        release = true;
    }

    if (b2 == 0x2A || b2 == 0x46) {
        scancode_sequence_t seq = {0};
        seq.prefix = SCANCODE_PREFIX_E0;
        seq.length = 3;
        for (size_t i = 0; i < 3; i++) {
            seq.bytes[i] = kbd_peek(i + 1);
        }

        virtual_key_t vk = VK_NONE;
        if (try_parse_sequence(&seq, &vk)) {
            keyboard_handle_key(vk, false);
        }

        buffer_remove_start(4);
        return;
    }

    if (b2 == 0xB7) {
        uint8_t b3 = kbd_peek(2);
        if (b3 == 0xE0) {
            keyboard_handle_key(VK_SNAPSHOT, true);
            buffer_remove_start(4);
            return;
        } else {
            keyboard_handle_key(VK_SNAPSHOT_CONTROL, true);
            buffer_remove_start(2);
            return;
        }
    }

    if (e0_code < 0x80) {
        virtual_key_t vk = scancode_e0_to_vk_quick[e0_code];
        if (vk != VK_NONE) {
            keyboard_handle_key(vk, release);
        }
    }
    buffer_remove_start(2);
}

/**
 * @brief Handles standard 1-byte scancodes.
 */
void handle_standard_scancode(void) {
    uint8_t code = kbd_peek(0);
    bool release = false;
    if (code & 0x80) {
        code &= 0x7F;
        release = true;
    }
    
    if (code < 0x80) {
        virtual_key_t vk = scancode_to_vk_quick[code];
        if (vk != VK_NONE) {
            keyboard_handle_key(vk, release);
        }
    }
}

/**
 * @brief Periodic update function that processes the scancode buffer.
 */
void keyboard_update(void) {
    while (kbd_available() > 0) {
        uint8_t b1 = kbd_peek(0);
        
        if (b1 == SCANCODE_PREFIX_E1) {
            if (kbd_available() < 6) return;
            handle_e1_sequence();
            buffer_remove_start(6);
            continue;
        } 
        
        if (b1 == SCANCODE_PREFIX_E0) {
            if (kbd_available() < 2) return;
            uint8_t b2 = kbd_peek(1);
            if (b2 == 0x2A || b2 == 0x46) {
                if (kbd_available() < 4) return;
            }
            if (b2 == 0xB7) {
                if (kbd_available() < 3) return;
            }
            handle_e0_prefix();
            continue;
        }

        handle_standard_scancode();
        buffer_remove_start(1);
    }
}
