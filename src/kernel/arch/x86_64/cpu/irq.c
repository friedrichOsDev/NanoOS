/**
 * @file irq.c
 * @brief IRQ Setup
 */

#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/cpu/idt.h>
#include <arch/x86_64/cpu/interrupts.h>
#include <arch/x86_64/cpu/irq.h>
#include <arch/x86_64/cpu/pic.h>
#include <arch/x86_64/drivers/serial.h>
#include <stdint.h>

void irq_init() {
    pic_disable();

    static const uint64_t irq_table[16] = {
        (uint64_t)irq0, (uint64_t)irq1, (uint64_t)irq2, (uint64_t)irq3,
        (uint64_t)irq4, (uint64_t)irq5, (uint64_t)irq6, (uint64_t)irq7,
        (uint64_t)irq8, (uint64_t)irq9, (uint64_t)irq10, (uint64_t)irq11,
        (uint64_t)irq12, (uint64_t)irq13, (uint64_t)irq14, (uint64_t)irq15};

    serial_printf(COM1, "IRQ: setting IRQ entries\n");
    for (uint8_t i = 0; i < 16; i++) {
        idt_set_gate(32 + i, irq_table[i], GDT_SEL_KERN_CODE, 0, IDT_GATE_INTERRUPT);
    }
}