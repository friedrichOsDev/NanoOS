#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KEYBOARD_BUFFER_SIZE 256

// keyboard buffer
typedef struct {
    char buffer[KEYBOARD_BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
} keyboard_buffer_t;

// keyboard global states
typedef struct {
    bool shift;
    bool ctrl;
    bool alt;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
    uint8_t last_prefix;
} keyboard_state_t;

// normal key (a, b, ...)
typedef struct {
    uint8_t scancode;
    char ascii_normal;
    char ascii_shift;
} key_t;

// action key (SHIFT, CAPSLOCK, ...)
typedef struct {
    uint8_t scancode;
    void (*action_make)(void);
    void (*action_break)(void);
} action_key_t;

// extended scancode key (UP, DOWN, WIN, ...)
typedef struct {
    uint8_t prefix;
    uint8_t scancode;
    void (*action_make)(void);
    void (*action_break)(void);
} extended_key_t;

// map of all keys
typedef struct {
    const char* language;
    const key_t* keys;
    uint16_t keys_count;
    const action_key_t* action_keys;
    uint16_t action_keys_count;
    const extended_key_t* extended_keys;
    uint16_t extended_keys_count;
} keymap_t;

// map of all languages
// languages i want to support
// us international
// de german (DONE)
// en english
typedef struct {
    const keymap_t* const* keymaps;
    uint16_t keymaps_count;
    const char* const* language_names;
    uint16_t language_names_count;
    keyboard_state_t keyboard_state;
    const keymap_t* active_keymap;
} keyboard_t;

extern const keymap_t keymap_de;
// extern const keymap_t keymap_us;
// extern const keymap_t keymap_en;
extern keyboard_t keyboard;

// action key handlers
void esc_make(void);
void esc_break(void);
void oem_4_make(void);
void oem_4_break(void);
void oem_6_make(void);
void oem_6_break(void);
void backspace_make(void);
void backspace_break(void);
void tab_make(void);
void tab_break(void);
void oem_1_make(void);
void oem_1_break(void);
void oem_plus_make(void);
void oem_plus_break(void);
void enter_make(void);
void enter_break(void);
void lctrl_make(void);
void lctrl_break(void);
void oem_3_make(void);
void oem_3_break(void);
void oem_7_make(void);
void oem_7_break(void);
void oem_5_make(void);
void oem_5_break(void);
void lshift_make(void);
void lshift_break(void);
void oem_2_make(void);
void oem_2_break(void);
void oem_comma_make(void);
void oem_comma_break(void);
void oem_period_make(void);
void oem_period_break(void);
void oem_minus_make(void);
void oem_minus_break(void);
void rshift_make(void);
void rshift_break(void);
void multiply_make(void);
void multiply_break(void);
void lmenu_make(void);
void lmenu_break(void);
void capslock_make(void);
void capslock_break(void);
void f1_make(void);
void f1_break(void);
void f2_make(void);
void f2_break(void);
void f3_make(void);
void f3_break(void);
void f4_make(void);
void f4_break(void);
void f5_make(void);
void f5_break(void);
void f6_make(void);
void f6_break(void);
void f7_make(void);
void f7_break(void);
void f8_make(void);
void f8_break(void);
void f9_make(void);
void f9_break(void);
void f10_make(void);
void f10_break(void);
void numlock_make(void);
void numlock_break(void);
void scrolllock_make(void);
void scrolllock_break(void);
void num_home_make(void);
void num_home_break(void);
void num_up_make(void);
void num_up_break(void);
void num_prior_make(void);
void num_prior_break(void);
void subtract_make(void);
void subtract_break(void);
void num_left_make(void);
void num_left_break(void);
void num_clear_make(void);
void num_clear_break(void);
void num_right_make(void);
void num_right_break(void);
void add_make(void);
void add_break(void);
void num_end_make(void);
void num_end_break(void);
void num_down_make(void);
void num_down_break(void);
void num_next_make(void);
void num_next_break(void);
void num_insert_make(void);
void num_insert_break(void);
void num_delete_make(void);
void num_delete_break(void);
void snapshot_make(void);
void snapshot_break(void);
// 0x55 doesn't exists
void oem_102_make(void);
void oem_102_break(void);
void f11_make(void);
void f11_break(void);
void f12_make(void);
void f12_break(void);
// 0xE0 prefix
void num_enter_make(void);
void num_enter_break(void);
void rctrl_make(void);
void rctrl_break(void);
void divide_make(void);
void divide_break(void);
void rmenu_make(void);
void rmenu_break(void);
void home_make(void);
void home_break(void);
void up_make(void);
void up_break(void);
void prior_make(void);
void prior_break(void);
void left_make(void);
void left_break(void);
void right_make(void);
void right_break(void);
void end_make(void);
void end_break(void);
void down_make(void);
void down_break(void);
void next_make(void);
void next_break(void);
void insert_make(void);
void insert_break(void);
void delete_make(void);
void delete_break(void);
void lwin_make(void);
void lwin_break(void);
void rwin_make(void);
void rwin_break(void);
void apps_make(void);
void apps_break(void);
// 0xE1 prefix
void pause_make(void);
void pause_break(void);

// keyboard handlers
void scancode_handler(uint8_t scancode);
void keyboard_callback(uint32_t irq);

// keyboard init
void keyboard_init(const char* language);
char keyboard_getchar(void);

#endif // KEYBOARD_H