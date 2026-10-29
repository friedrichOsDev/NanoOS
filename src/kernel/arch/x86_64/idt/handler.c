/**
 * @file handler.c
 * @brief Implementierung der Handler für ISRs, IRQs und IPIs.
 * @author friedrichOsDev
 */

#include "handler.h"
#include <arch/arch.h>
#include <core/scheduler.h>
#include <kernel/panic.h>
#include <kernel/sync.h>
#include <pch.h>
#include "../apic/apic.h"

static isr_handler_t isr_handlers[ISR_COUNT];
static irq_handler_t irq_handlers[IRQ_COUNT];
static spinlock_t exception_print_lock = SPINLOCK_INIT;

void isr_install_handler(int isr, isr_handler_t handler) {
    if (isr < 0 || isr >= ISR_COUNT) {
        serial_printf("HANDLER: Invalid ISR number %d\n", isr);
        return;
    }
    isr_handlers[isr] = handler;
}

void irq_install_handler(int irq, irq_handler_t handler) {
    if (irq < 0 || irq >= IRQ_COUNT) {
        serial_printf("HANDLER: Invalid IRQ number %d\n", irq);
        return;
    }
    irq_handlers[irq] = handler;
}

void isr_handler(struct registers *regs) {
    uint64_t rf = spinlock_acquire_irqsave(&exception_print_lock);
    if (regs->int_no < ISR_COUNT) {
        serial_printf("\n=== EXCEPTION %lld (Error Code: %llx) ===\n", regs->int_no, regs->err_code);
    }

    if (regs->int_no < ISR_COUNT && isr_handlers[regs->int_no]) {
        isr_handlers[regs->int_no](regs);
        spinlock_release_irqrestore(&exception_print_lock, rf);
        return;
    }

    /* Core & Thread Kontext auslesen */
    cpu_local_t *cpu = smp_get_current_cpu();
    int cpu_id = cpu ? (int)cpu->cpu_id : -1;
    const char *thread_name = (cpu && cpu->current_thread) ? cpu->current_thread->name : "unknown/none";

    serial_printf("CPU Core: %d | Thread: %s\n", cpu_id, thread_name);

    /* Steuerregister auslesen */
    uint64_t cr2, cr3;
    ASM_CODE("mov %%cr2, %0" : "=r"(cr2));
    ASM_CODE("mov %%cr3, %0" : "=r"(cr3));

    if (regs->int_no == 14) {
        serial_printf("Faulting Address (CR2): %016llx\n", cr2);
    }
    serial_printf("Page Table Base (CR3) : %016llx\n", cr3);

    /* Register-Dump der allgemeinen Register */
    serial_printf("RAX: %016llx RBX: %016llx RCX: %016llx RDX: %016llx\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
    serial_printf("RSI: %016llx RDI: %016llx RBP: %016llx RSP: %016llx\n", regs->rsi, regs->rdi, regs->rbp, regs->rsp);
    serial_printf("R8 : %016llx R9 : %016llx R10: %016llx R11: %016llx\n", regs->r8, regs->r9, regs->r10, regs->r11);
    serial_printf("R12: %016llx R13: %016llx R14: %016llx R15: %016llx\n", regs->r12, regs->r13, regs->r14, regs->r15);
    serial_printf("RIP: %016llx CS : %016llx RFLAGS: %016llx\n", regs->rip, regs->cs, regs->rflags);

    spinlock_release_irqrestore(&exception_print_lock, rf);
    panic();
}

void irq_handler(struct registers *regs) {
    uint64_t irq = regs->int_no - 32;

    if (irq < IRQ_COUNT && irq_handlers[irq]) {
        irq_handlers[irq](regs);
        lapic_eoi();
        return;
    }

    if (irq >= IRQ_COUNT) {
        serial_printf("HANDLER: Invalid IRQ number %lld\n", irq);
    }
    lapic_eoi();
}

void reschedule_ipi_handler(struct registers *regs) {
    (void)regs;
    scheduler_schedule();
    lapic_eoi();
}

void stop_ipi_handler(struct registers *regs) {
    (void)regs;
    lapic_eoi();
    ASM_CODE("cli");
    while (1) {
        ASM_CODE("hlt");
    }
}

void tlb_shootdown_ipi_handler(struct registers *regs) {
    (void)regs;
    uint64_t cr3;
    ASM_CODE("mov %%cr3, %0; mov %0, %%cr3" : "=r"(cr3)::"memory");
    lapic_eoi();
}

void lapic_timer_handler(struct registers *regs) {
    (void)regs;
    scheduler_tick();
    lapic_eoi();
}
