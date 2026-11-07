/**
 * @file arch.h
 * @brief Architecture-specific functions and data structures.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

[[maybe_unused]] static inline void arch_sti() { ASM_CODE("sti"); }
[[maybe_unused]] static inline void arch_cli() { ASM_CODE("cli"); }
[[maybe_unused]] static inline void arch_hlt() { ASM_CODE("hlt"); }
[[maybe_unused]] static inline void arch_pause() { ASM_CODE("pause"); }
[[maybe_unused]] static inline void arch_idle() {
    arch_sti();
    arch_hlt();
}

// acpi on x86_64
kerr_t acpi_init(virt_addr_t rsdp_addr);
void acpi_power_off();

// hpet on x86_64
void hpet_init();
uint64_t hpet_read_counter();
void hpet_udelay(uint64_t microseconds);
void hpet_mdelay(uint64_t milliseconds);
uint64_t hpet_uptime_ms();
uint64_t hpet_uptime_us();

// cross cpu/apic
kerr_t apic_init();
void lapic_init(uint32_t target_hz);
bool is_apic_initialized();
void arch_ipi_reschedule();
void arch_ipi_stop_others();
void arch_tlb_flush_local(virt_addr_t vaddr);
void arch_tlb_shootdown();

// io bus on x86_64
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
void outw(uint16_t port, uint16_t val);
uint16_t inw(uint16_t port);
void outl(uint16_t port, uint32_t val);
uint32_t inl(uint16_t port);

// serial port on x86_64
void serial_init();
kerr_t serial_putc(char c);
kerr_t serial_puts(const char *str);
void serial_printf(const char *format, ...);
void vserial_printf(const char *format, va_list args);

// fpu on x86_64
void fpu_init();
void fpu_state_init(void *fpu_buf);

// gdt on x86_64
void gdt_init();
void gdt_init_core(size_t cpu_id, uintptr_t kernel_stack);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void gdt_set_tss_gate(int num, uintptr_t base, uint32_t limit);
void gdt_set_tss_kernel_stack(size_t cpu_id, uint64_t stack_top);

// idt on x86_64
void idt_init();
void idt_set_gate(uint8_t num, uint64_t base, uint16_t selector, uint8_t ist, uint8_t flags);

// irq on x86_64
void irq_init();
irq_flags_t arch_irq_save();
void arch_irq_restore(irq_flags_t flags);

// isr/irq handlers on x86_64
void isr_install_handler(int isr, isr_handler_t handler);
void irq_install_handler(int irq, irq_handler_t handler);

/** @brief Per-CPU local data structure containing core state and scheduling context. */
typedef struct cpu_local {
    uint32_t cpu_id;             /**< Logical CPU identifier (0 = BSP, 1 = AP1, ...) */
    uint32_t lapic_id;           /**< Physical Local APIC ID from ACPI MADT */
    uint64_t kernel_stack;       /**< Base address of this CPU's kernel stack */
    volatile bool online;        /**< Flag indicating whether the CPU core is active */
    thread_t *current_thread;    /**< Pointer to the thread currently executing on this CPU */
    thread_t *idle_thread;       /**< Dedicated idle thread for this CPU */
    thread_t *thread_to_enqueue; /**< Temporary thread buffer for scheduler enqueue operations */
} cpu_local_t;

// cpu/smp
kerr_t smp_init();
uint32_t smp_get_cpu_id();
size_t smp_get_cpu_count();
cpu_local_t *smp_get_current_cpu();
cpu_local_t *smp_get_cpu_by_id(uint32_t cpu_id);

// context switch symbols on x86_64
extern void switch_context(uint64_t *prev_rsp_ptr, uint64_t next_rsp, void *prev_fpu_state, void *next_fpu_state);
extern void thread_entry_stub();
