/**
 * @file handler.h
 * @brief Interrupt Handler for ISRs and IRQs (Header)
 * @author friedrichOsDev
 */

#pragma once
#include <stdint.h>

struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

typedef void (*isr_handler_t)(struct registers *regs);
typedef isr_handler_t irq_handler_t;

void isr_install_handler(int isr, isr_handler_t handler);
void irq_install_handler(int irq, isr_handler_t handler);
void isr_handler(struct registers *regs);
void irq_handler(struct registers *regs);