/*
 * @file pic.c
 * @brief Programmable Interrupt Controller (PIC)
 * @author friedrichOsDev
 */

#include <pic.h>
#include <io.h>
#include <serial.h>

/*
 * TODO: Replace the 0x... magic numbers with defined constants
 */

/*
 * Remap the PIC to avoid conflicts with CPU exceptions
 * @param void
 */
void pic_remap(void) {
    serial_printf("PIC: remapping\n");
    
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);

    serial_printf("PIC: remapped\n");
}