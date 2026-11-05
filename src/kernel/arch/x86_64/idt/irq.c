/**
 * @file irq.c
 * @brief IRQ implementation.
 * @author friedrichOsDev
 */

#include "../apic/pic.h"
#include "../gdt/gdt.h"
#include "../idt/idt.h"
#include "../idt/int_sym.h"
#include <arch/arch.h>
#include <pch.h>

/** @brief Initializes hardware IRQs by disabling the legacy PIC and mapping IRQ gates (vectors 32-47) in the IDT. */
void irq_init() {
    pic_disable();

    static const uint64_t irq_table[16] = {
        (uint64_t)irq0, (uint64_t)irq1, (uint64_t)irq2, (uint64_t)irq3,
        (uint64_t)irq4, (uint64_t)irq5, (uint64_t)irq6, (uint64_t)irq7,
        (uint64_t)irq8, (uint64_t)irq9, (uint64_t)irq10, (uint64_t)irq11,
        (uint64_t)irq12, (uint64_t)irq13, (uint64_t)irq14, (uint64_t)irq15};

    for (uint8_t i = 0; i < 16; i++) {
        idt_set_gate((uint8_t)(32 + i), irq_table[i], GDT_SEL_KERN_CODE, 0, IDT_GATE_INTERRUPT);
    }
}

/**
 * @brief Saves the current interrupt flag state and disables hardware interrupts.
 *
 * @return The saved RFLAGS register state prior to disabling interrupts.
 */
irq_flags_t arch_irq_save() {
    irq_flags_t rflags;
    ASM_CODE("pushfq\n\t"
             "pop %0\n\t"
             "cli"
             : "=r"(rflags)
             :
             : "memory");
    return rflags;
}

/**
 * @brief Restores the CPU interrupt flag state from a previously saved RFLAGS value.
 *
 * @param[in] rflags The RFLAGS state to restore.
 */
void arch_irq_restore(irq_flags_t rflags) {
    ASM_CODE("pushq %0\n\t"
             "popfq"
             :
             : "r"(rflags)
             : "memory", "cc");
}
