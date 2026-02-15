/*
 * @file handler.c
 * @brief Interrupt Service Routine (ISR) and Interrupt Request (IRQ) handlers
 * @author friedrichOsDev
 */

#include <handler.h>
#include <serial.h>
#include <io.h>

static irq_handler_t irq_handlers[16];
static isr_handler_t isr_handlers[32];

/*
 * Install a custom handler for a specific IRQ
 * @param irq The IRQ number (0 - 15)
 * @param handler The function pointer to the custom handler
 */
void irq_install_handler(int irq, irq_handler_t handler) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = handler;
    } else {
        serial_printf("Error: Invalid IRQ number %d! Must be between 0 and 15.\n", irq);
    }
}

/*
 * Install a custom handler for a specific ISR
 * @param isr The ISR number (0 - 31)
 * @param handler The function pointer to the custom handler
 */
void isr_install_handler(int isr, isr_handler_t handler) {
    if (isr >= 0 && isr < 32) {
        isr_handlers[isr] = handler;
    } else {
        serial_printf("Error: Invalid ISR number %d! Must be between 0 and 31.\n", isr);
    }
}

/*
 * Common handler for IRQs
 * @param regs The register state passed by the assembly ISR
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

/*
 * Common handler for ISRs
 * @param regs The register state passed by the assembly ISR
 */
void isr_handler(struct registers *regs) {
    if (regs->int_no < 32) {
        serial_printf("Exception: %d, Error Code: %d\n", regs->int_no, regs->err_code);
    }
    isr_handler_t handler = isr_handlers[regs->int_no];
    if (handler) {
        handler(regs);
    } else {
        serial_printf("Unhandled Exception: %d\n", regs->int_no);
        while (1);
    }
}