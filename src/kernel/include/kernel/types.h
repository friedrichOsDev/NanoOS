/**
 * @file types.h
 * @brief Types for the kernel.
 * @author friedrichOsDev
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// mm
typedef uint64_t phys_addr_t;
typedef uintptr_t virt_addr_t;

// sched forward declarations
struct thread;
typedef struct thread thread_t;
struct process;
typedef struct process process_t;

/** @brief Structure representing saved CPU registers during an interrupt. */
struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8; /**< Saved general-purpose registers (R8-R15) */
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;    /**< Saved general-purpose registers (RAX-RBP) */
    uint64_t int_no;                               /**< Interrupt vector number */
    uint64_t err_code;                             /**< CPU error code (or dummy value) */
    uint64_t rip;                                  /**< Instruction pointer at the time of the interrupt */
    uint64_t cs;                                   /**< Code segment selector */
    uint64_t rflags;                               /**< CPU RFLAGS register */
    uint64_t rsp;                                  /**< Stack pointer at the time of the interrupt */
    uint64_t ss;                                   /**< Stack segment selector */
};

typedef void (*isr_handler_t)(struct registers *regs);
typedef isr_handler_t irq_handler_t;
typedef uint64_t irq_flags_t;
