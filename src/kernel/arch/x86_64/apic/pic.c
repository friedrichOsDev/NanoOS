/**
 * @file pic.c
 * @brief Deaktivierung des veralteten 8259 PIC Controllers.
 * @author friedrichOsDev
 */

#include "pic.h"
#include <arch/arch.h>

void pic_disable() {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}
