/**
 * @file handler.h
 * @brief Schnittstelle für Interrupt-Service-Routinen (ISRs), Hardware-IRQs und Inter-Processor-Interrupts (IPIs).
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/**
 * @brief Registerzustand der CPU, der beim Auftreten eines Interrupts auf dem Stack gesichert wird.
 */
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

/** @brief Funktionszeiger-Typ für CPU-Exception-Handler (ISRs) */
typedef void (*isr_handler_t)(struct registers *regs);

/** @brief Funktionszeiger-Typ für IRQ-Handler */
typedef isr_handler_t irq_handler_t;

/**
 * @brief Registriert einen benutzerdefinierten Handler für eine bestimmte CPU-Exception (ISR).
 * @param isr Vektornummer der ISR (0 bis 31).
 * @param handler Zeiger auf die auszuführende Handler-Funktion.
 */
void isr_install_handler(int isr, isr_handler_t handler);

/**
 * @brief Registriert einen benutzerdefinierten Handler für einen Hardware-Interrupt (IRQ).
 * @param irq IRQ-Nummer (0 bis 47).
 * @param handler Zeiger auf die auszuführende Handler-Funktion.
 */
void irq_install_handler(int irq, irq_handler_t handler);

/**
 * @brief Zentraler C-Dispatcher für CPU-Exceptions.
 * @param regs Zeiger auf den gesicherten Registerzustand auf dem Stack.
 */
void isr_handler(struct registers *regs);

/**
 * @brief Zentraler C-Dispatcher für Hardware-Interrupts (IRQs).
 * @param regs Zeiger auf den gesicherten Registerzustand auf dem Stack.
 */
void irq_handler(struct registers *regs);

/**
 * @brief Handler für Rescheduling-IPIs zur Auslösung einer Thread-Umschaltung auf dem Zielkern.
 * @param regs Zeiger auf den gesicherten Registerzustand auf dem Stack.
 */
void reschedule_ipi_handler(struct registers *regs);

/**
 * @brief Handler für Stop-IPIs zum Herunterfahren/Anhalten von Nebenkernen (HALT-Schleife).
 * @param regs Zeiger auf den gesicherten Registerzustand auf dem Stack.
 */
void stop_ipi_handler(struct registers *regs);

/**
 * @brief Handler für TLB-Shootdown-IPIs zum Ungültigmachen von Paging-Caches über Kernelkerne hinweg.
 * @param regs Zeiger auf den gesicherten Registerzustand auf dem Stack.
 */
void tlb_shootdown_ipi_handler(struct registers *regs);

/**
 * @brief Handler für den periodischen Local APIC Timer-Interrupt (Scheduler-Tick).
 * @param regs Zeiger auf den gesicherten Registerzustand auf dem Stack.
 */
void lapic_timer_handler(struct registers *regs);