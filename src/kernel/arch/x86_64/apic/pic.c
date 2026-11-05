/**
 * @file pic.c
 * @brief Disable 8259 PIC.
 * @author friedrichOsDev
 */

#include "pic.h"
#include <arch/arch.h>

/** @brief Disable the PIC */
void pic_disable() {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}
