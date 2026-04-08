/**
 * @file keyboard.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <layouts.h>
#include <handler.h>

#define KBD_BUFFER_SIZE 1024

/**
 * @brief Ring buffer for raw scancodes received from the keyboard controller.
 */
typedef struct {
    uint32_t buffer[KBD_BUFFER_SIZE]; /**< The scancode data. */
    uint16_t head; /**< Index where the next byte will be written. */
    uint16_t tail; /**< Index where the next byte will be read. */
} kbd_scancode_buffer_t;

/**
 * @brief Ring buffer for translated Unicode characters.
 */
typedef struct {
    uint32_t buffer[KBD_BUFFER_SIZE]; /**< The Unicode character data. */
    uint16_t head; /**< Index where the next character will be written. */
    uint16_t tail; /**< Index where the next character will be read. */
} kbd_data_buffer_t;

/**
 * @brief Represents the current state of keyboard modifiers and locks.
 */
typedef struct {
    bool shift;
    bool ctrl;
    bool alt;
    bool alt_gr;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
} keyboard_state_t;

/**
 * @brief States for the keyboard scancode state machine.
 */
typedef enum {
    KBD_STATE_NORMAL,
    KBD_STATE_E0,
    KBD_STATE_E1,
    KBD_STATE_E1_COLLECT
} kbd_parse_state_t;

/**
 * @brief Function pointer type for virtual key actions.
 * 
 * @param release True if the key was released, false if pressed.
 * @return True if the event was handled, false otherwise.
 */
typedef bool (*vk_action_t)(bool release);

void keyboard_init(const keyboard_layout_t* layout);
void keyboard_callback(struct registers *regs);

uint32_t keyboard_get_unicode(void);
void keyboard_map_function_to_vk(virtual_key_t vk, vk_action_t action);
void restore_default_map(void);
void restore_default_mapping(virtual_key_t vk);

void buffer_push(void* buffer, uint16_t* head, uint16_t tail, size_t buffer_size, uint32_t value);
uint32_t buffer_pop(void* buffer, uint16_t* head, uint16_t* tail, size_t buffer_size);
bool buffer_remove_start(uint16_t count);

bool action_escape(bool release);
bool action_f1(bool release);
bool action_f2(bool release);
bool action_f3(bool release);
bool action_f4(bool release);
bool action_f5(bool release);
bool action_f6(bool release);
bool action_f7(bool release);
bool action_f8(bool release);
bool action_f9(bool release);
bool action_f10(bool release);
bool action_f11(bool release);
bool action_f12(bool release);
bool action_snapshot(bool release);
bool action_snapshot_ctrl(bool release);
bool action_snapshot_shift(bool release);
bool action_snapshot_alt(bool release);
bool action_scroll(bool release);
bool action_pause(bool release);
bool action_pause_control(bool release);
bool action_backspace(bool release);
bool action_tab(bool release);
bool action_return(bool release);
bool action_capital(bool release);
bool action_lshift(bool release);
bool action_rshift(bool release);
bool action_lcontrol(bool release);
bool action_rcontrol(bool release);
bool action_lwin(bool release);
bool action_rwin(bool release);
bool action_lmenu(bool release);
bool action_rmenu(bool release);
bool action_apps(bool release);
bool action_insert(bool release);
bool action_home(bool release);
bool action_prior(bool release);
bool action_delete(bool release);
bool action_end(bool release);
bool action_next(bool release);
bool action_left(bool release);
bool action_up(bool release);
bool action_right(bool release);
bool action_down(bool release);
bool action_numlock(bool release);
bool action_clear(bool release);

void keyboard_handle_special(virtual_key_t vk, bool release);
void keyboard_handle_num_key(virtual_key_t vk, bool release);
bool vk_is_alpha(virtual_key_t vk);
bool vk_is_dead(virtual_key_t vk);
bool vk_is_special(virtual_key_t vk);
bool vk_is_num_key(virtual_key_t vk);
uint32_t keyboard_get_combined_char(uint32_t dead_char, uint32_t base_char);
void keyboard_merge_dead_key(virtual_key_t dead_vk, virtual_key_t vk);
uint32_t get_unicode_for_state(virtual_key_t vk, keyboard_state_t state);
void keyboard_handle_key(virtual_key_t vk, bool release);

bool try_parse_sequence(const scancode_sequence_t* seq, virtual_key_t* out_vk);
void handle_e1_sequence(void);
void handle_e0_prefix(void);
void handle_standard_scancode(void);
void keyboard_update(void);
