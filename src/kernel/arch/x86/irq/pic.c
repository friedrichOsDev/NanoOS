/**
 * @file pic.c
 * @author friedrichOsDev
 */

#include <pic.h>
#include <io.h>
#include <serial.h>

/**
 * @brief Remaps the Programmable Interrupt Controllers (PICs).
 * 
 * Moves the IRQ base vectors from the default (0x08-0x0F) to 0x20-0x2F 
 * to avoid conflicts with CPU exceptions.
 */
void pic_remap(void) {
    serial_printf("PIC: remapping\n");
    
    // ICW1: Start initialization
    outb(PIC1_COMMAND, ICW1_INIT);
    outb(PIC2_COMMAND, ICW1_INIT);

    // ICW2: Vector offsets (0x20 for Master, 0x28 for Slave)
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    // ICW3: Cascading
    outb(PIC1_DATA, 0x04); // Master has slave on IRQ2
    outb(PIC2_DATA, 0x02); // Slave identity

    // ICW4: Environment info
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // Mask: Enable all interrupts
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);

    serial_printf("PIC: remapped\n");
}