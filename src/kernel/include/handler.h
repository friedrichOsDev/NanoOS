/*
 * @file handler.h
 * @brief Header file for Interrupt Service Routine (ISR) and Interrupt Request (IRQ) handlers
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/*
 * Register state structure passed to handlers
 */
struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

typedef void (*isr_handler_t)(struct registers *regs);
typedef isr_handler_t irq_handler_t;

void irq_install_handler(int irq, irq_handler_t handler);
void isr_install_handler(int isr, isr_handler_t handler);
void irq_handler(struct registers *regs);
void isr_handler(struct registers *regs);