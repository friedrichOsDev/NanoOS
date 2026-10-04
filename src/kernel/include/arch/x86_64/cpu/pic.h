/**
 * @file pic.h
 * @brief Legacy PIC disabling for APIC mode
 */

#pragma once

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define ICW1_INIT 0x11
#define ICW4_8086 0x01

/**
 * @brief Masks all legacy PIC lines to avoid conflicts with APIC routing
 */
void pic_disable();