/**
 * @file irq.c
 * @brief Einrichtung der Hardware-Interrupt-Lines (IRQs) in der IDT.
 * @author friedrichOsDev
 */

#include "../apic/pic.h"
#include "../gdt/gdt.h"
#include "../idt/idt.h"
#include "../idt/int_sym.h"
#include <arch/arch.h>
#include <pch.h>

void irq_init() {
    pic_disable();

    /* Zuordnungstabelle der ersten 16 IRQ-Assembly-Stubs */
    static const uint64_t irq_table[16] = {
        (uint64_t)irq0, (uint64_t)irq1, (uint64_t)irq2, (uint64_t)irq3,
        (uint64_t)irq4, (uint64_t)irq5, (uint64_t)irq6, (uint64_t)irq7,
        (uint64_t)irq8, (uint64_t)irq9, (uint64_t)irq10, (uint64_t)irq11,
        (uint64_t)irq12, (uint64_t)irq13, (uint64_t)irq14, (uint64_t)irq15};

    serial_printf("IRQ: setting IRQ entries\n");
    for (uint8_t i = 0; i < 16; i++) {
        idt_set_gate((uint8_t)(32 + i), irq_table[i], GDT_SEL_KERN_CODE, 0, IDT_GATE_INTERRUPT);
    }
}

/**
 * @brief Liest das aktuelle RFLAGS-Register aus und deaktiviert Interrupts per CLI-Instruktion.
 * @return Der Wert des RFLAGS-Registers vor dem Aufruf von CLI.
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
 * @brief Stellt den im RFLAGS-Register gespeicherten Zustand (inkl. Interrupt-Flag) wieder her.
 * @param rflags Zu wiederherstellender RFLAGS-Wert.
 */
void arch_irq_restore(irq_flags_t rflags) {
    ASM_CODE("pushq %0\n\t"
             "popfq"
             :
             : "r"(rflags)
             : "memory", "cc");
}
