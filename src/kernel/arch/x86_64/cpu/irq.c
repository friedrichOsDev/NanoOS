/**
 * @file irq.c
 * @brief 64-bit Interrupt Hardware Request Setup
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/idt.h>
#include <arch/x86_64/cpu/interrupts.h>
#include <arch/x86_64/cpu/irq.h>
#include <arch/x86_64/cpu/pic.h>
#include <arch/x86_64/drivers/serial.h>
#include <stdint.h>

/**
 * Initializes the IRQ
 */
void irq_init() {
    pic_disable();
    uint64_t irq_table[] = {
        (uint64_t)irq0,  (uint64_t)irq1,  (uint64_t)irq2,  (uint64_t)irq3,
        (uint64_t)irq4,  (uint64_t)irq5,  (uint64_t)irq6,  (uint64_t)irq7,
        (uint64_t)irq8,  (uint64_t)irq9,  (uint64_t)irq10, (uint64_t)irq11,
        (uint64_t)irq12, (uint64_t)irq13, (uint64_t)irq14, (uint64_t)irq15};

    serial_printf(COM1, "IRQ: set IRQ entries\n");
    for (int i = 0; i < 16; i++) {
        idt_set_gate(32 + i, irq_table[i], 0x08, 0, 0x8E);
    }
}
