/**
 * @file pic.c
 * @brief PIC remapping
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/pic.h>
#include <arch/x86_64/drivers/serial.h>
#include <lib/io.h>

void pic_remap() {
    serial_printf(COM1, "PIC: remap\n");

    outb(PIC1_COMMAND, ICW1_INIT);
    outb(PIC2_COMMAND, ICW1_INIT);

    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);

}
