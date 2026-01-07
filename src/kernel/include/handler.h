#ifndef HANDLER_H
#define HANDLER_H

#include <stdint.h>

typedef void (*isr_handler_t)(uint32_t);
typedef isr_handler_t irq_handler_t;

void irq_install_handler(int irq, irq_handler_t handler);
void isr_install_handler(int isr, isr_handler_t handler);
void irq_handler(uint32_t irq);
void isr_handler(uint32_t isr);

#endif // HANDLER_H