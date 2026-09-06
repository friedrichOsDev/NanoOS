/**
 * @file handler.c
 * @brief Interrupt Handler for ISRs and IRQs
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/apic.h>
#include <arch/x86_64/cpu/handler.h>
#include <arch/x86_64/cpu/hpet.h>
#include <arch/x86_64/cpu/smp.h>
#include <arch/x86_64/drivers/serial.h>
#include <core/panic.h>
#include <core/scheduler.h>

static isr_handler_t isr_handlers[32];
static irq_handler_t irq_handlers[48];

/**
 * Installs a handler for a given ISR
 * @param isr The number of the ISR
 * @param handler The handler function for the ISR
 */
void isr_install_handler(int isr, isr_handler_t handler) {
    if (isr < 0 || isr >= 32) {
        serial_printf(COM1, "HANDLER: Invalid ISR number %d\n", isr);
    } else {
        isr_handlers[isr] = handler;
    }
}

/**
 * Installs a handler for a given IRQ
 * @param irq The number of the IRQ
 * @param handler The handler function for the IRQ
 */
void irq_install_handler(int irq, irq_handler_t handler) {
    if (irq < 0 || irq >= 48) {
        serial_printf(COM1, "HANDLER: Invalid IRQ number %d\n", irq);
    } else {
        irq_handlers[irq] = handler;
    }
}

static void print_backtrace(uint64_t rbp) {
    serial_printf(COM1, "Call Trace:\n");
    uint64_t *frame = (uint64_t *)rbp;
    int depth = 0;

    while (frame && (uint64_t)frame >= KERNEL_SPACE_START && depth < 10) {
        uint64_t next_rbp = frame[0];
        uint64_t rip = frame[1];

        if (rip == 0) break;

        serial_printf(COM1, "  [%d] RIP: %016llx (RBP: %016llx)\n", depth, rip, (uint64_t)frame);

        if (next_rbp <= (uint64_t)frame) break;
        frame = (uint64_t *)next_rbp;
        depth++;
    }
}

/**
 * Standard ISR handler
 * @param regs CPU registers
 */
void isr_handler(struct registers *regs) {
    if (regs->int_no < 32) {
        serial_printf(COM1, "\n=== EXCEPTION %lld (Error Code: %llx) ===\n", 
                      regs->int_no, regs->err_code);
    }

    isr_handler_t handler = isr_handlers[regs->int_no];
    if (handler) {
        handler(regs);
    } else {
        // 1. Core & Thread Context
        cpu_local_t *cpu = smp_get_current_cpu();
        int cpu_id = cpu ? (int)cpu->cpu_id : -1;
        const char *thread_name = (cpu && cpu->current_thread) ? cpu->current_thread->name : "unknown/none";

        serial_printf(COM1, "CPU Core: %d | Thread: %s\n", cpu_id, thread_name);

        // 2. Control Registers
        uint64_t cr2, cr3;
        __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
        __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));

        if (regs->int_no == 14) {
            serial_printf(COM1, "Faulting Address (CR2): %016llx\n", cr2);
        }
        serial_printf(COM1, "Page Table Base (CR3) : %016llx\n", cr3);

        // 3. General Registers
        serial_printf(COM1, "RAX: %016llx RBX: %016llx RCX: %016llx RDX: %016llx\n",
                      regs->rax, regs->rbx, regs->rcx, regs->rdx);
        serial_printf(COM1, "RSI: %016llx RDI: %016llx RBP: %016llx RSP: %016llx\n",
                      regs->rsi, regs->rdi, regs->rbp, regs->rsp);
        serial_printf(COM1, "R8 : %016llx R9 : %016llx R10: %016llx R11: %016llx\n",
                      regs->r8, regs->r9, regs->r10, regs->r11);
        serial_printf(COM1, "R12: %016llx R13: %016llx R14: %016llx R15: %016llx\n",
                      regs->r12, regs->r13, regs->r14, regs->r15);
        serial_printf(COM1, "RIP: %016llx CS : %016llx RFLAGS: %016llx\n",
                      regs->rip, regs->cs, regs->rflags);

        // 4. Stack Trace
        print_backtrace(regs->rbp);

        panic("Unhandled exception", regs->int_no);
    }
}

/**
 * Standard IRQ handler
 * @param regs CPU registers
 */
void irq_handler(struct registers *regs) {
    uint64_t irq = regs->int_no - 32;

    lapic_eoi();

    if (irq < 48) {
        irq_handler_t handler = irq_handlers[irq];
        if (handler) {
            handler(regs);
        }
        return;
    }

    serial_printf(COM1, "HANDLER: Invalid IRQ number %lld\n", irq);
}

/**
 * IPI reschedule handler
 * @param regs CPU registers
 */
void reschedule_ipi_handler(struct registers *regs) {
    (void)regs;
    lapic_eoi();
    scheduler_schedule();
}

/**
 * IPI stop handler (halts the receiving CPU core)
 * @param regs CPU registers
 */
void stop_ipi_handler(struct registers *regs) {
    (void)regs;
    lapic_eoi();
    __asm__ __volatile__("cli");
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

/**
 * IPI TLB shootdown handler
 * @param regs CPU registers
 */
void tlb_shootdown_ipi_handler(struct registers *regs) {
    (void)regs;
    lapic_eoi();
    // Flush the entire TLB by reloading CR3
    uint64_t cr3;
    __asm__ __volatile__("mov %%cr3, %0; mov %0, %%cr3" : "=r" (cr3) :: "memory");
}

/**
 * LAPIC timer interrupt handler
 * @note fires independently on each core
 */
void lapic_timer_handler(struct registers *regs) {
    (void)regs;
    lapic_eoi();
    scheduler_tick();
}