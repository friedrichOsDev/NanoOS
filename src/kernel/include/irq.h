#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

void enable_interrupts(void);
void disable_interrupts(void);

void irq_init(void);

#endif // IRQ_H