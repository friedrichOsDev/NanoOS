/**
 * @file pic.c
 * @brief PIC remapping
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/pic.h>
#include <lib/io.h>

/**
 * Disables the PIC to use APIC
 */
void pic_disable() {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}