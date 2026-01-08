#include <handler.h>
#include <io.h>
#include <log.h>

static irq_handler_t irq_routines[16];
static isr_handler_t exception_handlers[32];

void irq_install_handler(int irq, irq_handler_t handler) {
    if (irq >= 32 && irq < 48) {
        irq_routines[irq - 32] = handler;
    }
}

void isr_install_handler(int isr, isr_handler_t handler) {
    if (isr < 32) {
        exception_handlers[isr] = handler;
    }
}

void irq_handler(uint32_t irq) {
    // irq is the interrupt number (32-47)
    irq_handler_t handler = irq_routines[irq - 32];
    if (handler) {
        handler(irq);
    }

    // send EOI (End of Interrupt) to the PICs
    if (irq >= 40) { 
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

void isr_handler(uint32_t int_no) {
    if (exception_handlers[int_no]) {
        exception_handlers[int_no](int_no);
    }
    
    log_cpu_exception(int_no, "CPU Exception occurred.");
    while(1);
}
