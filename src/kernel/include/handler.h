/**
 * @file handler.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/**
 * @brief Structure representing the CPU registers pushed onto the stack during an interrupt.
 */
struct registers {
    uint32_t ds;                                     /**< Data segment selector */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /**< Pushed by pusha */
    uint32_t int_no, err_code;                       /**< Interrupt number and error code (if applicable) */
    uint32_t eip, cs, eflags, useresp, ss;           /**< Pushed by the processor automatically */
};

/** @brief Function pointer type for ISR and IRQ handlers. */
typedef void (*isr_handler_t)(struct registers *regs);
/** @brief Alias for isr_handler_t used for IRQs. */
typedef isr_handler_t irq_handler_t;

void irq_install_handler(int irq, irq_handler_t handler);
void isr_install_handler(int isr, isr_handler_t handler);
void irq_handler(struct registers *regs);
void isr_handler(struct registers *regs);