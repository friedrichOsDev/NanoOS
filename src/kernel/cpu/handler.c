/*
 * @file handler.c
 * @brief CPU interrupt and exception handler management
 * @author friedrichOsDev
 */

#include <handler.h>
#include <io.h>
#include <log.h>

static irq_handler_t irq_routines[16];
static isr_handler_t exception_handlers[32];

/*
 * A function to install a custom IRQ handler
 * @param irq: The IRQ number (32 to 47)
 * @param handler: The handler function pointer
 */
void irq_install_handler(int irq, irq_handler_t handler) {
    if (irq >= 32 && irq < 48) {
        irq_routines[irq - 32] = handler;
    }
}

/*
 * A function to install a custom ISR handler
 * @param isr: The ISR number (0 to 31)
 * @param handler: The handler function pointer
 */
void isr_install_handler(int isr, isr_handler_t handler) {
    if (isr < 32) {
        exception_handlers[isr] = handler;
    }
}

/*
 * The IRQ handler called by the assembly interrupt stubs
 * @param irq: The IRQ number
 */
void irq_handler(uint32_t irq) {
    irq_handler_t handler = irq_routines[irq - 32];
    if (handler) {
        handler(irq);
    }

    if (irq >= 40) { 
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

/*
 * The ISR handler called by the assembly interrupt stubs
 * @param int_no: The interrupt number
 */
void isr_handler(uint32_t int_no) {
    if (exception_handlers[int_no]) {
        exception_handlers[int_no](int_no);
    }
    
    log_cpu_exception(int_no, "CPU Exception occurred.");
    while(1);
}
