/**
 * @file handler.c
 * @author friedrichOsDev
 */

#include <handler.h>
#include <serial.h>
#include <io.h>

/** @brief Array of registered IRQ handlers. */
static irq_handler_t irq_handlers[16];
/** @brief Array of registered ISR handlers. */
static isr_handler_t isr_handlers[32];

/**
 * @brief Installs a custom handler for a specific IRQ.
 * 
 * @param irq The IRQ number (0-15).
 * @param handler The function to call when the IRQ occurs.
 */
void irq_install_handler(int irq, irq_handler_t handler) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = handler;
    } else {
        serial_printf("Error: Invalid IRQ number %d! Must be between 0 and 15.\n", irq);
    }
}

/**
 * @brief Installs a custom handler for a specific ISR (exception).
 * 
 * @param isr The ISR number (0-31).
 * @param handler The function to call when the exception occurs.
 */
void isr_install_handler(int isr, isr_handler_t handler) {
    if (isr >= 0 && isr < 32) {
        isr_handlers[isr] = handler;
    } else {
        serial_printf("Error: Invalid ISR number %d! Must be between 0 and 31.\n", isr);
    }
}

/**
 * @brief The common IRQ handler called from assembly stubs.
 * 
 * Sends EOI to the PICs and dispatches to the registered handler.
 * @param regs The CPU register state at the time of the interrupt.
 */
void irq_handler(struct registers *regs) {
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);

    irq_handler_t handler = irq_handlers[regs->int_no - 32];
    if (handler) {
        handler(regs);
    }
}

/**
 * @brief The common ISR handler called from assembly stubs.
 * 
 * Dispatches to the registered exception handler or hangs on unhandled exceptions.
 * @param regs The CPU register state at the time of the exception.
 */
void isr_handler(struct registers *regs) {
    if (regs->int_no < 32) {
        serial_printf("Exception: %d, Error Code: %d\n", regs->int_no, regs->err_code);
    }
    isr_handler_t handler = isr_handlers[regs->int_no];
    if (handler) {
        handler(regs);
    } else {
        while (1);
    }
}