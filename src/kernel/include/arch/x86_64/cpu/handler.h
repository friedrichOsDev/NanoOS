/**
 * @file handler.h
 * @brief Interrupt Handler for ISRs and IRQs
 */

#pragma once

#include <stdint.h>

/**
 * @brief CPU Register State saved on interrupt stack
 */
struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

typedef void (*isr_handler_t)(struct registers *regs);
typedef isr_handler_t irq_handler_t;

/**
 * @brief Installs a custom ISR handler
 * @param isr ISR vector index (0-31)
 * @param handler Callback function pointer
 */
void isr_install_handler(int isr, isr_handler_t handler);

/**
 * @brief Installs a custom IRQ handler
 * @param irq IRQ line index (0-47)
 * @param handler Callback function pointer
 */
void irq_install_handler(int irq, irq_handler_t handler);

/**
 * @brief Central C ISR dispatch handler
 * @param regs Pointer to saved register state on stack
 */
void isr_handler(struct registers *regs);

/**
 * @brief Central C IRQ dispatch handler
 * @param regs Pointer to saved register state on stack
 */
void irq_handler(struct registers *regs);

/**
 * @brief Handles rescheduling IPI interrupts
 * @param regs Pointer to saved register state on stack
 */
void reschedule_ipi_handler(struct registers *regs);

/**
 * @brief Handles core shutdown IPI interrupts
 * @param regs Pointer to saved register state on stack
 */
void stop_ipi_handler(struct registers *regs);

/**
 * @brief Handles TLB shootdown IPI interrupts
 * @param regs Pointer to saved register state on stack
 */
void tlb_shootdown_ipi_handler(struct registers *regs);

/**
 * @brief Handles local APIC timer interrupts
 * @param regs Pointer to saved register state on stack
 */
void lapic_timer_handler(struct registers *regs);