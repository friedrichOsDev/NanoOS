/**
 * @file handler.c
 * @brief Interrupt Handler for ISRs and IRQs
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/handler.h>
#include <arch/x86_64/cpu/apic.h>
#include <arch/x86_64/cpu/hpet.h>
#include <arch/x86_64/drivers/serial.h>
#include <core/panic.h>

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

/**
 * Standard ISR handler
 * @param regs CPU registers
 */
void isr_handler(struct registers *regs) {
    if (regs->int_no < 32) {
        serial_printf(COM1, "Exception: %lld, Error Code: %lld\n", regs->int_no, regs->err_code);
    }

    isr_handler_t handler = isr_handlers[regs->int_no];
    if (handler) {
        handler(regs);
    } else {
        serial_printf(COM1, "RAX: %016llx RBX: %016llx RCX: %016llx RDX: %016llx\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
        serial_printf(COM1, "RSI: %016llx RDI: %016llx RBP: %016llx RSP: %016llx\n", regs->rsi, regs->rdi, regs->rbp, regs->rsp);
        serial_printf(COM1, "R8 : %016llx R9 : %016llx R10: %016llx R11: %016llx\n", regs->r8,  regs->r9,  regs->r10, regs->r11);
        serial_printf(COM1, "R12: %016llx R13: %016llx R14: %016llx R15: %016llx\n", regs->r12, regs->r13, regs->r14, regs->r15);
        serial_printf(COM1, "RIP: %016llx CS : %016llx RFLAGS: %016llx\n", regs->rip, regs->cs, regs->rflags);

        panic("Unhandled exception", regs->int_no);
    }
}

/**
 * Standard IRQ handler
 * @param regs CPU registers
 */
void irq_handler(struct registers *regs) {
    uint64_t irq = regs->int_no - 32;

    if (irq >= 48) {
        serial_printf(COM1, "HANDLER: Invalid IRQ number %lld\n", irq);
    } else {
        irq_handler_t handler = irq_handlers[irq];
        if (handler) {
            handler(regs);
        }
    }

    lapic_eoi();
}