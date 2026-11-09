/**
 * @file ps2.h
 * @brief Internal PS/2 controller and device interfaces.
 * @author friedrichOsDev
 */
#pragma once
#include <drivers/hid/hid.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_STATUS_INPUT_FULL 0x02
#define PS2_STATUS_AUXILIARY 0x20
#define PS2_STATUS_TIMEOUT 0x40
#define PS2_STATUS_PARITY 0x80

kerr_t ps2_controller_init();
void ps2_controller_deinit();
kerr_t ps2_register_driver();
void ps2_keyboard_byte(uint8_t byte);
void ps2_mouse_byte(uint8_t byte);
void ps2_mouse_set_id(uint8_t id);
void ps2_irq_handler(struct registers *regs);
void ps2_keyboard_reset_parser();
void ps2_mouse_reset_parser();
