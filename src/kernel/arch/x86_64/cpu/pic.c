/**
 * @file pic.c
 * @brief Legacy PIC disabling for APIC mode
 */

#include <arch/x86_64/cpu/pic.h>
#include <lib/io.h>

void pic_disable() {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}