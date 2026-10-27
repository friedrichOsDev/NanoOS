/**
 * @file interrupts.h
 * @brief Deklarationen für Interrupt- (ISR) und Hardware-Interrupt-Stubs (IRQ) in Assembler.
 * @author friedrichOsDev
 */

#pragma once

/* ==============================================================================
 * CPU-Ausnahmen & ISR-Stubs (Vektoren 0 bis 31)
 * ============================================================================== */

extern void isr0();  /**< #DE: Division Error */
extern void isr1();  /**< #DB: Debug Exception */
extern void isr2();  /**< Non-Maskable Interrupt (NMI) */
extern void isr3();  /**< #BP: Breakpoint */
extern void isr4();  /**< #OF: Overflow */
extern void isr5();  /**< #BR: BOUND Range Exceeded */
extern void isr6();  /**< #UD: Invalid Opcode */
extern void isr7();  /**< #NM: Device Not Available (No Math Coprocessor) */
extern void isr8();  /**< #DF: Double Fault (mit Error Code) */
extern void isr9();  /**< Coprocessor Segment Overrun */
extern void isr10(); /**< #TS: Invalid TSS (mit Error Code) */
extern void isr11(); /**< #NP: Segment Not Present (mit Error Code) */
extern void isr12(); /**< #SS: Stack-Segment Fault (mit Error Code) */
extern void isr13(); /**< #GP: General Protection Fault (mit Error Code) */
extern void isr14(); /**< #PF: Page Fault (mit Error Code) */
extern void isr15(); /**< Reserviert */
extern void isr16(); /**< #MF: x87 FPU Floating-Point Error */
extern void isr17(); /**< #AC: Alignment Check (mit Error Code) */
extern void isr18(); /**< #MC: Machine Check */
extern void isr19(); /**< #XM/#XF: SIMD Floating-Point Exception */
extern void isr20(); /**< #VE: Virtualization Exception */
extern void isr21(); /**< #CP: Control Protection Exception */
extern void isr22(); /**< Reserviert */
extern void isr23(); /**< Reserviert */
extern void isr24(); /**< Reserviert */
extern void isr25(); /**< Reserviert */
extern void isr26(); /**< Reserviert */
extern void isr27(); /**< Reserviert */
extern void isr28(); /**< Hypervisor Injection Exception */
extern void isr29(); /**< VMM Communication Exception */
extern void isr30(); /**< Security Exception */
extern void isr31(); /**< Reserviert */

/* ==============================================================================
 * Hardware-Interrupts / IRQ-Stubs (Vektoren 32 bis 47)
 * ============================================================================== */

extern void irq0();  /**< IRQ 0: System-Timer (PIT) */
extern void irq1();  /**< IRQ 1: Tastatur */
extern void irq2();  /**< IRQ 2: Kaskadierung für PIC2 */
extern void irq3();  /**< IRQ 3: Serieller Port 2/4 (COM2/COM4) */
extern void irq4();  /**< IRQ 4: Serieller Port 1/3 (COM1/COM3) */
extern void irq5();  /**< IRQ 5: Soundkarte / LPT2 */
extern void irq6();  /**< IRQ 6: Diskette (Floppy Controller) */
extern void irq7();  /**< IRQ 7: Paralleler Port (LPT1) / Spurious IRQ */
extern void irq8();  /**< IRQ 8: Echtzeituhr (RTC) */
extern void irq9();  /**< IRQ 9: ACPI / Frei verwendbar */
extern void irq10(); /**< IRQ 10: Frei / PCI Peripherie */
extern void irq11(); /**< IRQ 11: Frei / PCI Peripherie */
extern void irq12(); /**< IRQ 12: PS/2-Maus */
extern void irq13(); /**< IRQ 13: Koprozessor / FPU Exception */
extern void irq14(); /**< IRQ 14: Primärer ATA/IDE-Kanal */
extern void irq15(); /**< IRQ 15: Sekundärer ATA/IDE-Kanal */

/**
 * @brief Lädt die IDT-Adresse und das Limit mittels `lidt`-Assemblerbefehl in das IDTR-Register.
 * @param idt_ptr Virtuelle Adresse der `idt_ptr`-Struktur.
 */
extern void idt_load(uint64_t idt_ptr);

/** @brief Assembly-Stub zur Behandlung von Spurious Interrupts. */
extern void spurious_handler_stub();

/** @brief Assembly-Stub zur Behandlung von Reschedule-IPIs. */
extern void ipi_reschedule_stub();

/** @brief Assembly-Stub zur Behandlung von Stop-IPIs. */
extern void ipi_stop_stub();

/** @brief Assembly-Stub zur Behandlung von TLB-Shootdown-IPIs. */
extern void ipi_tlb_shootdown_stub();

/** @brief Assembly-Stub zur Behandlung des LAPIC Timer-Interrupts. */
extern void lapic_timer_stub();
