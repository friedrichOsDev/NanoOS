#include <keyboard.h>
#include <handler.h>
#include <io.h>
#include <string.h>

// keyboard predefinitions
static keyboard_buffer_t kb_buffer = {.head = 0, .tail = 0};
keyboard_t keyboard;

// support functions
// --- kb buffer functions ---
static inline bool kb_buffer_empty() {
    return kb_buffer.head == kb_buffer.tail;
}

static inline bool kb_buffer_full() {
    return ((kb_buffer.head + 1) & (KEYBOARD_BUFFER_SIZE - 1)) == kb_buffer.tail;
}

static void kb_buffer_push(char c) {
    if (kb_buffer_full()) {
        kb_buffer.tail = (kb_buffer.tail + 1) & (KEYBOARD_BUFFER_SIZE - 1);
    }
    kb_buffer.buffer[kb_buffer.head] = c;
    kb_buffer.head = (kb_buffer.head + 1) & (KEYBOARD_BUFFER_SIZE - 1);
}

static int kb_buffer_pop() {
    if (kb_buffer_empty()) return -1;
    char c = kb_buffer.buffer[kb_buffer.tail];
    kb_buffer.tail = (kb_buffer.tail + 1) & (KEYBOARD_BUFFER_SIZE - 1);
    return (int)c;
}

// --- find key in keymap functions ---
const key_t* find_key(uint8_t scancode, const keymap_t* keymap) {
    if (!keymap || !keymap->keys) return NULL;
    for (uint16_t i = 0; i < keymap->keys_count; i++) {
        if (keymap->keys[i].scancode == scancode) {
            return &keymap->keys[i];
        }
    }
    return NULL;
}

const action_key_t* find_action_key(uint8_t scancode, const keymap_t* keymap) {
    if (!keymap || !keymap->action_keys) return NULL;
    for (uint16_t i = 0; i < keymap->action_keys_count; i++) {
        if (keymap->action_keys[i].scancode == scancode) {
            return &keymap->action_keys[i];
        }
    }
    return NULL;
}

const extended_key_t* find_extended_key(uint8_t prefix, uint8_t scancode, const keymap_t* keymap) {
    if (!keymap || !keymap->extended_keys) return NULL;
    for (uint16_t i = 0; i < keymap->extended_keys_count; i++) {
        if (keymap->extended_keys[i].prefix == prefix && keymap->extended_keys[i].scancode == scancode) {
            return &keymap->extended_keys[i];
        }
    }
    return NULL;
}

// --- handler utilities ---
static inline bool is_make_code(uint8_t scancode) {
    return (scancode & 0x80) == 0;
}

static inline uint8_t base_scancode(uint8_t scancode) {
    return scancode & 0x7F;
}

// --- ctype ---
static inline bool is_alpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

// --- language utilities ---
static inline bool keyboard_is_de() {
    return strcmp(keyboard.active_keymap->language, "de") == 0;
}

static inline bool keyboard_is_us() {
    return strcmp(keyboard.active_keymap->language, "us") == 0;
}

static inline bool keyboard_is_en() {
    return strcmp(keyboard.active_keymap->language, "en") == 0;
}

static inline bool is_uppercase_active(char c) {
    if (is_alpha(c)) {
        return keyboard.keyboard_state.shift ^ keyboard.keyboard_state.caps_lock;
    } else {
        return keyboard.keyboard_state.shift;
    }
}

static inline void push_if_de(char c) {
    if (keyboard_is_de()) kb_buffer_push(c);
}

static inline void push_if_de_shift(char c_normal, char c_shift) {
    if (!keyboard_is_de()) return;
    if (is_uppercase_active(c_normal)) {
        kb_buffer_push(c_shift);
    } else {
        kb_buffer_push(c_normal);
    }
}

static inline void push_if_us(char c) {
    if (keyboard_is_us()) kb_buffer_push(c);
}

static inline void push_if_us_shift(char c_normal, char c_shift) {
    if (!keyboard_is_us()) return;
    if (is_uppercase_active(c_normal)) {
        kb_buffer_push(c_shift);
    } else {
        kb_buffer_push(c_normal);
    }
}

static inline void push_if_en(char c) {
    if (keyboard_is_en()) kb_buffer_push(c);
}

static inline void push_if_en_shift(char c_normal, char c_shift) {
    if (!keyboard_is_en()) return;
    if (is_uppercase_active(c_normal)) {
        kb_buffer_push(c_shift);
    } else {
        kb_buffer_push(c_normal);
    }
}

static inline bool num_active() {
    return keyboard.keyboard_state.num_lock;
}

static inline void push_if_num(char c, void (*function)(void)) {
    if (num_active()) kb_buffer_push(c);
    else if (function) function();
}

// key handlers
// --- empty handler ---
#define empty_handler(name) \
    void name##_make() {} \
    void name##_break() {}

// action key handlers
// special keys
void esc_make(void) {}
void esc_break(void) {}

void backspace_make(void) { kb_buffer_push('\b'); }
void backspace_break(void) {}

void tab_make(void) { kb_buffer_push('\t'); }
void tab_break(void) {}

void enter_make(void) { kb_buffer_push('\n'); }
void enter_break(void) {}

// modifier keys
void lctrl_make(void) { keyboard.keyboard_state.ctrl = true; }
void lctrl_break(void) { keyboard.keyboard_state.ctrl = false; }

void rctrl_make(void) { keyboard.keyboard_state.ctrl = true; }
void rctrl_break(void) { keyboard.keyboard_state.ctrl = false; }

void lshift_make(void) { keyboard.keyboard_state.shift = true; }
void lshift_break(void) { keyboard.keyboard_state.shift = false; }

void rshift_make(void) { keyboard.keyboard_state.shift = true; }
void rshift_break(void) { keyboard.keyboard_state.shift = false; }

void lmenu_make(void) { keyboard.keyboard_state.alt = true; }
void lmenu_break(void) { keyboard.keyboard_state.alt = false; }

void rmenu_make(void) { keyboard.keyboard_state.alt = true; }
void rmenu_break(void) { keyboard.keyboard_state.alt = false; }

void capslock_make(void) { keyboard.keyboard_state.caps_lock = !keyboard.keyboard_state.caps_lock; }
void capslock_break(void) {}
void numlock_make(void) { keyboard.keyboard_state.num_lock = !keyboard.keyboard_state.num_lock; }
void numlock_break(void) {}
void scrolllock_make(void) { keyboard.keyboard_state.scroll_lock = !keyboard.keyboard_state.scroll_lock; }
void scrolllock_break(void) {}

// oem keys
void oem_1_make(void) {} // ü not supported :(
void oem_1_break(void) {}

void oem_2_make(void) { push_if_de_shift('#', '\''); } // #
void oem_2_break(void) {}

void oem_3_make(void) {} // ö not supported :(
void oem_3_break(void) {}

void oem_4_make(void) {
    if (!keyboard_is_de()) return;
    if (keyboard.keyboard_state.shift) {
        kb_buffer_push('?');
    } // ß not supported :(
}
void oem_4_break(void) {}

void oem_5_make(void) {
    if (!keyboard_is_de()) return;
    if (keyboard.keyboard_state.shift) {
        // ° not supported :(
    } else {
        kb_buffer_push('^');
    }
}
void oem_5_break(void) {}

void oem_6_make(void) {
    if (!keyboard_is_de()) return;
    if (keyboard.keyboard_state.shift) {
        kb_buffer_push('`');
    } // ´ not supported :(
}
void oem_6_break(void) {}

void oem_7_make(void) {} // ä not supported :(
void oem_7_break(void) {}

void oem_plus_make(void) { push_if_de_shift('+', '*'); } // +
void oem_plus_break(void) {}

void oem_minus_make(void) { push_if_de_shift('-', '_'); } // -
void oem_minus_break(void) {}

void oem_comma_make(void) { push_if_de_shift(',', ';'); } // ,
void oem_comma_break(void) {}

void oem_period_make(void) { push_if_de_shift('.', ':'); } // .
void oem_period_break(void) {}

void oem_102_make(void) { push_if_de_shift('<', '>'); } // <
void oem_102_break(void) {}

// num keys
void num_home_make(void) { push_if_num('7', home_make); }
void num_home_break(void) {}
void num_up_make(void) { push_if_num('8', up_make); }
void num_up_break(void) {}
void num_prior_make(void) { push_if_num('9', prior_make); }
void num_prior_break(void) {}
void subtract_make(void) { push_if_de('-'); }
void subtract_break(void) {}
void num_left_make(void) { push_if_num('4', left_make); }
void num_left_break(void) {}
void num_clear_make(void) { if (num_active()) kb_buffer_push('5'); }
void num_clear_break(void) {}
void num_right_make(void) { push_if_num('6', right_make); }
void num_right_break(void) {}
void add_make(void) { push_if_de('+'); }
void add_break(void) {}
void num_end_make(void) { push_if_num('1', end_make); }
void num_end_break(void) {}
void num_down_make(void) { push_if_num('2', down_make); }
void num_down_break(void) {}
void num_next_make(void) { push_if_num('3', next_make); }
void num_next_break(void) {}
void num_insert_make(void) { push_if_num('0', insert_make); }
void num_insert_break(void) {}
void num_delete_make(void) { push_if_num(',', delete_make);}
void num_delete_break(void) {}
void num_enter_make(void) { push_if_de('\n'); }
void num_enter_break(void) {}
void divide_make(void) { push_if_de('/'); }
void divide_break(void) {}
void multiply_make(void) { push_if_de('*'); }
void multiply_break(void) {}

empty_handler(snapshot)
empty_handler(home)
empty_handler(up)
empty_handler(prior)
empty_handler(left)
empty_handler(right)
empty_handler(end)
empty_handler(down)
empty_handler(next)
empty_handler(insert)
empty_handler(delete)
empty_handler(lwin)
empty_handler(rwin)
empty_handler(apps)
empty_handler(pause)
empty_handler(f1)
empty_handler(f2)
empty_handler(f3)
empty_handler(f4)
empty_handler(f5)
empty_handler(f6)
empty_handler(f7)
empty_handler(f8)
empty_handler(f9)
empty_handler(f10)
empty_handler(f11)
empty_handler(f12)

// scancode handler
void scancode_handler(uint8_t scancode) {
    // handle prefix bytes
    if (scancode == 0xE0 || scancode == 0xE1) {
        keyboard.keyboard_state.last_prefix = scancode;
        return;
    }

    bool is_make = is_make_code(scancode);
    uint8_t base = base_scancode(scancode);

    // if there was a last prefix, treat as extended
    if (keyboard.keyboard_state.last_prefix == 0xE0 || keyboard.keyboard_state.last_prefix == 0xE1) {
        uint8_t prefix = keyboard.keyboard_state.last_prefix;
        keyboard.keyboard_state.last_prefix = 0x00; // consume
        const extended_key_t* ek = find_extended_key(prefix, base, keyboard.active_keymap);
        if (ek) {
            if (is_make) {
                if (ek->action_make) ek->action_make();
            } else {
                if (ek->action_break) ek->action_break();
            }
            return;
        }
    }

    // first check action keys
    const action_key_t* ak = find_action_key(base, keyboard.active_keymap);
    if (ak) {
        if (is_make) {
            if (ak->action_make) ak->action_make();
        } else {
            if (ak->action_break) ak->action_break();
        }
        return;
    }

    // normal keys
    const key_t* k = find_key(base, keyboard.active_keymap);
    if (k) {
        if (is_make) {
            push_if_de_shift(k->ascii_normal, k->ascii_shift);
            push_if_us_shift(k->ascii_normal, k->ascii_shift);
            push_if_en_shift(k->ascii_normal, k->ascii_shift);
        }
    }
}

// keyboard callback
void keyboard_callback(uint32_t irq) {
    (void)irq;
    uint8_t scancode = inb(0x60);
    scancode_handler(scancode);
}

// keyboard init
void keyboard_init(const char* language) {
    irq_install_handler(33, keyboard_callback);

    // build keymap list
    const keymap_t* keymaps[] = {
        &keymap_de,
        // &keymap_us,
        // &keymap_en
    };

    const char* language_names[] = {
        "de",
        // "us",
        // "en"
    };

    keyboard.keymaps = keymaps;
    keyboard.keymaps_count = (uint16_t)(sizeof(keymaps) / sizeof(keymaps[0]));
    keyboard.language_names = language_names;
    keyboard.language_names_count = (uint16_t)(sizeof(language_names) / sizeof(language_names[0]));

    // set default keymap
    keyboard.active_keymap = &keymap_de;

    // set active keymap
    for (uint16_t i = 0; i < keyboard.keymaps_count; i++) {
        if (strcmp(keyboard.keymaps[i]->language, language) == 0) {
            keyboard.active_keymap = keyboard.keymaps[i];
            break;
        }
    }

    // reset keyboard state
    keyboard.keyboard_state.shift = false;
    keyboard.keyboard_state.ctrl = false;
    keyboard.keyboard_state.alt = false;
    keyboard.keyboard_state.caps_lock = false;
    keyboard.keyboard_state.num_lock = false;
    keyboard.keyboard_state.scroll_lock = false;
    keyboard.keyboard_state.last_prefix = 0x00;

    // clear buffer
    kb_buffer.head = 0;
    kb_buffer.tail = 0;
    memset(kb_buffer.buffer, 0, KEYBOARD_BUFFER_SIZE);
}

char keyboard_getchar(void) {
    if (kb_buffer_empty()) return (char)0;
    int c = kb_buffer_pop();
    if (c == -1) return (char)0;
    return (char)c;
}
