/**
 * @file handler.c
 * @brief Interrupts handler implementatiohns for ISRs, IRQs and IPIs.
 * @author friedrichOsDev
 */

#include "handler.h"
#include "../apic/apic.h"
#include <arch/arch.h>
#include <kernel/panic.h>
#include <kernel/sync.h>
#include <pch.h>
#include <sched/sched.h>

static isr_handler_t isr_handlers[ISR_COUNT];
static irq_handler_t irq_handlers[IRQ_COUNT];
static spinlock_t exception_print_lock = SPINLOCK_INIT;

/**
 * @brief Installs a custom callback handler for a specific CPU exception / ISR.
 *
 * @param[in] isr ISR vector number.
 * @param[in] handler Function pointer to the ISR handler callback.
 */
void isr_install_handler(int isr, isr_handler_t handler) {
    if (isr < 0 || isr >= ISR_COUNT) {
        log_error("Cannot install an ISR handler for an invalid ISR number %d", isr);
        return;
    }
    isr_handlers[isr] = handler;
}

/**
 * @brief Installs a custom callback handler for a specific hardware IRQ line.
 *
 * @param[in] irq IRQ index (relative to base vector 32).
 * @param[in] handler Function pointer to the IRQ handler callback.
 */
void irq_install_handler(int irq, irq_handler_t handler) {
    if (irq < 0 || irq >= IRQ_COUNT) {
        log_error("Cannot install an IRQ handler for an invalid IRQ number %d", irq);
        return;
    }
    irq_handlers[irq] = handler;
}

/**
 * @brief Global dispatcher for CPU exceptions and interrupts (ISRs 0–31).
 *
 * @param[in] regs Pointer to the saved CPU register state frame.
 */
void isr_handler(struct registers *regs) {
    irq_flags_t rf = spinlock_acquire_irqsave(&exception_print_lock);
    if (regs->int_no < ISR_COUNT) {
        log_error("EXCEPTION %lld (Error Code: %llx)", regs->int_no, regs->err_code);
    }

    if (regs->int_no < ISR_COUNT && isr_handlers[regs->int_no]) {
        isr_handlers[regs->int_no](regs);
        spinlock_release_irqrestore(&exception_print_lock, rf);
        return;
    }

    cpu_local_t *cpu = smp_get_current_cpu();
    int cpu_id = cpu ? (int)cpu->cpu_id : -1;
    const char *thread_name = (cpu && cpu->current_thread) ? cpu->current_thread->name : "unknown/none";

    log_debug("CPU Core: %d | Thread: %s", cpu_id, thread_name);

    uint64_t cr2, cr3;
    ASM_CODE("mov %%cr2, %0" : "=r"(cr2));
    ASM_CODE("mov %%cr3, %0" : "=r"(cr3));

    if (regs->int_no == 14) {
        log_debug("Faulting Address (CR2): %016llx", cr2);
    }
    log_debug("Page Table Base (CR3) : %016llx", cr3);
    log_registers(regs);

    spinlock_release_irqrestore(&exception_print_lock, rf);
    panic();
}

/**
 * @brief Global dispatcher for external hardware interrupts (IRQs).
 *
 * @param[in] regs Pointer to the saved CPU register state frame.
 */
void irq_handler(struct registers *regs) {
    uint64_t irq = regs->int_no - 32;

    if (irq < IRQ_COUNT && irq_handlers[irq]) {
        irq_handlers[irq](regs);
        lapic_eoi();
        return;
    }

    if (irq >= IRQ_COUNT) {
        log_error("Cannot handle IRQ number (%d) greater than %d", irq, IRQ_COUNT);
    }
    lapic_eoi();
}

/**
 * @brief Inter-Processor Interrupt (IPI) handler to trigger thread rescheduling.
 *
 * @param[in] regs Pointer to the saved CPU register state frame.
 */
void reschedule_ipi_handler(struct registers *regs) {
    (void)regs;

    /*
     * A reschedule can context-switch away from this interrupt frame.
     * Acknowledge the IPI before scheduling so the LAPIC does not keep this
     * vector in-service while the interrupted thread is suspended.
     */
    lapic_eoi();
    sched_schedule();
}

/**
 * @brief Inter-Processor Interrupt (IPI) handler to halt a target CPU core.
 *
 * @param[in] regs Pointer to the saved CPU register state frame.
 */
void stop_ipi_handler(struct registers *regs) {
    (void)regs;
    lapic_eoi();
    ASM_CODE("cli");
    while (1) {
        ASM_CODE("hlt");
    }
}

/**
 * @brief Inter-Processor Interrupt (IPI) handler to invalidate TLB entries across cores.
 *
 * @param[in] regs Pointer to the saved CPU register state frame.
 */
void tlb_shootdown_ipi_handler(struct registers *regs) {
    (void)regs;
    uint64_t cr3;
    ASM_CODE("mov %%cr3, %0; mov %0, %%cr3" : "=r"(cr3)::"memory");
    lapic_eoi();
}

/**
 * @brief Local APIC timer interrupt handler driving the kernel scheduler tick.
 *
 * @param[in] regs Pointer to the saved CPU register state frame.
 */
void lapic_timer_handler(struct registers *regs) {
    (void)regs;

    /*
     * sched_tick() may invoke the scheduler and switch to another thread.
     * Send EOI first; otherwise the timer interrupt can remain in-service
     * until this particular thread is scheduled again, stalling further
     * timer interrupts on this CPU.
     */
    lapic_eoi();
    sched_tick();
}
