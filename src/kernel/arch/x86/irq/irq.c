/**
 * @file irq.c
 * @author friedrichOsDev
 */

#include <irq.h>
#include <io.h>
#include <pic.h>
#include <idt.h>
#include <serial.h>
#include <kernel.h>
#include <interrupts.h>

/**
 * @brief Initializes the Interrupt Requests (IRQs).
 * 
 * Remaps the PICs and populates the IDT with IRQ stubs starting at 
 * interrupt vector 32.
 */
void irq_init(void) {
    pic_remap();

    serial_printf("IRQ: setting up IRQ gates\n");
    void *irq_table[] = {
        irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7,
        irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
    };

    for (int i = 0; i < 16; i++) {
        idt_set_gate(32 + i, (uint32_t)irq_table[i], 0x08, 0x8E);
    }
    init_state = INIT_IRQ;
}