/**
 * @file interrupts.h
 * @brief Deklarationen für Interrupt- (ISR) und Hardware-Interrupt-Stubs (IRQ) in Assembler.
 * @author friedrichOsDev
 */

#pragma once

/**
 * @brief Aktiviert Hardware-Interrupts global (`sti`).
 */
extern void idt_enable(void);

/**
 * @brief Deaktiviert Hardware-Interrupts global (`cli`).
 */
extern void idt_disable(void);

/* ==============================================================================
 * CPU-Ausnahmen & ISR-Stubs (Vektoren 0 bis 31)
 * ============================================================================== */

extern void isr0(void);   /**< #DE: Division Error */
extern void isr1(void);   /**< #DB: Debug Exception */
extern void isr2(void);   /**< Non-Maskable Interrupt (NMI) */
extern void isr3(void);   /**< #BP: Breakpoint */
extern void isr4(void);   /**< #OF: Overflow */
extern void isr5(void);   /**< #BR: BOUND Range Exceeded */
extern void isr6(void);   /**< #UD: Invalid Opcode */
extern void isr7(void);   /**< #NM: Device Not Available (No Math Coprocessor) */
extern void isr8(void);   /**< #DF: Double Fault (mit Error Code) */
extern void isr9(void);   /**< Coprocessor Segment Overrun */
extern void isr10(void);  /**< #TS: Invalid TSS (mit Error Code) */
extern void isr11(void);  /**< #NP: Segment Not Present (mit Error Code) */
extern void isr12(void);  /**< #SS: Stack-Segment Fault (mit Error Code) */
extern void isr13(void);  /**< #GP: General Protection Fault (mit Error Code) */
extern void isr14(void);  /**< #PF: Page Fault (mit Error Code) */
extern void isr15(void);  /**< Reserviert */
extern void isr16(void);  /**< #MF: x87 FPU Floating-Point Error */
extern void isr17(void);  /**< #AC: Alignment Check (mit Error Code) */
extern void isr18(void);  /**< #MC: Machine Check */
extern void isr19(void);  /**< #XM/#XF: SIMD Floating-Point Exception */
extern void isr20(void);  /**< #VE: Virtualization Exception */
extern void isr21(void);  /**< #CP: Control Protection Exception */
extern void isr22(void);  /**< Reserviert */
extern void isr23(void);  /**< Reserviert */
extern void isr24(void);  /**< Reserviert */
extern void isr25(void);  /**< Reserviert */
extern void isr26(void);  /**< Reserviert */
extern void isr27(void);  /**< Reserviert */
extern void isr28(void);  /**< Hypervisor Injection Exception */
extern void isr29(void);  /**< VMM Communication Exception */
extern void isr30(void);  /**< Security Exception */
extern void isr31(void);  /**< Reserviert */

/* ==============================================================================
 * Hardware-Interrupts / IRQ-Stubs (Vektoren 32 bis 47)
 * ============================================================================== */

extern void irq0(void);   /**< IRQ 0: System-Timer (PIT) */
extern void irq1(void);   /**< IRQ 1: Tastatur */
extern void irq2(void);   /**< IRQ 2: Kaskadierung für PIC2 */
extern void irq3(void);   /**< IRQ 3: Serieller Port 2/4 (COM2/COM4) */
extern void irq4(void);   /**< IRQ 4: Serieller Port 1/3 (COM1/COM3) */
extern void irq5(void);   /**< IRQ 5: Soundkarte / LPT2 */
extern void irq6(void);   /**< IRQ 6: Diskette (Floppy Controller) */
extern void irq7(void);   /**< IRQ 7: Paralleler Port (LPT1) / Spurious IRQ */
extern void irq8(void);   /**< IRQ 8: Echtzeituhr (RTC) */
extern void irq9(void);   /**< IRQ 9: ACPI / Frei verwendbar */
extern void irq10(void);  /**< IRQ 10: Frei / PCI Peripherie */
extern void irq11(void);  /**< IRQ 11: Frei / PCI Peripherie */
extern void irq12(void);  /**< IRQ 12: PS/2-Maus */
extern void irq13(void);  /**< IRQ 13: Koprozessor / FPU Exception */
extern void irq14(void);  /**< IRQ 14: Primärer ATA/IDE-Kanal */
extern void irq15(void);  /**< IRQ 15: Sekundärer ATA/IDE-Kanal */