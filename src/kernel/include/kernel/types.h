/**
 * @file types.h
 * @brief Alle Datentypen für meinen Kernel
 * @author friedrichOsDev
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// mm
typedef uint64_t phys_addr_t;
typedef uintptr_t virt_addr_t;

// handler registers and functions
struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8; /**< Gesicherte Allgemeine Register (R8-R15) */
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;    /**< Gesicherte Allgemeine Register (RAX-RBP) */
    uint64_t int_no;                               /**< Interrupt-Vektornummer */
    uint64_t err_code;                             /**< CPU-Fehlercode (oder Dummy-Wert) */
    uint64_t rip;                                  /**< Instruction Pointer zum Zeitpunkt des Interrupts */
    uint64_t cs;                                   /**< Code-Segment-Selektor */
    uint64_t rflags;                               /**< CPU RFLAGS-Register */
    uint64_t rsp;                                  /**< Stack Pointer zum Zeitpunkt des Interrupts */
    uint64_t ss;                                   /**< Stack-Segment-Selektor */
};

typedef void (*isr_handler_t)(struct registers *regs);
typedef isr_handler_t irq_handler_t;
typedef uint64_t irq_flags_t;
