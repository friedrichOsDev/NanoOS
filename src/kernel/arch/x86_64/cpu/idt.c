/**
 * @file idt.c
 * @brief IDT Setup
 */

#include <arch/x86_64/cpu/apic.h>
#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/cpu/idt.h>
#include <arch/x86_64/cpu/interrupts.h>
#include <arch/x86_64/drivers/serial.h>

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint64_t)&idt;

    static const uint64_t isr_table[32] = {
        (uint64_t)isr0, (uint64_t)isr1, (uint64_t)isr2, (uint64_t)isr3,
        (uint64_t)isr4, (uint64_t)isr5, (uint64_t)isr6, (uint64_t)isr7,
        (uint64_t)isr8, (uint64_t)isr9, (uint64_t)isr10, (uint64_t)isr11,
        (uint64_t)isr12, (uint64_t)isr13, (uint64_t)isr14, (uint64_t)isr15,
        (uint64_t)isr16, (uint64_t)isr17, (uint64_t)isr18, (uint64_t)isr19,
        (uint64_t)isr20, (uint64_t)isr21, (uint64_t)isr22, (uint64_t)isr23,
        (uint64_t)isr24, (uint64_t)isr25, (uint64_t)isr26, (uint64_t)isr27,
        (uint64_t)isr28, (uint64_t)isr29, (uint64_t)isr30, (uint64_t)isr31};

    serial_printf(COM1, "IDT: clearing all 256 entries\n");
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate((uint8_t)i, 0, 0, 0, 0);
    }

    for (int i = 0; i < 32; i++) {
        const uint8_t ist_index = (i == 8) ? 1 : 0; /* Use IST1 for Double Fault */
        idt_set_gate((uint8_t)i, isr_table[i], GDT_SEL_KERN_CODE, ist_index, IDT_GATE_INTERRUPT);
    }

    idt_set_gate(0xFF, (uint64_t)spurious_handler_stub, GDT_SEL_KERN_CODE, 0, IDT_GATE_INTERRUPT);
    idt_set_gate(IPI_RESCHEDULE_VECTOR, (uint64_t)ipi_reschedule_stub, GDT_SEL_KERN_CODE, 0, IDT_GATE_INTERRUPT);
    idt_set_gate(IPI_STOP_VECTOR, (uint64_t)ipi_stop_stub, GDT_SEL_KERN_CODE, 0, IDT_GATE_INTERRUPT);
    idt_set_gate(IPI_TLB_SHOOTDOWN_VECTOR, (uint64_t)ipi_tlb_shootdown_stub, GDT_SEL_KERN_CODE, 0, IDT_GATE_INTERRUPT);
    idt_set_gate(LAPIC_TIMER_VECTOR, (uint64_t)lapic_timer_stub, GDT_SEL_KERN_CODE, 0, IDT_GATE_INTERRUPT);

    serial_printf(COM1, "IDT: loading IDTR\n");
    idt_load((uint64_t)&idtp);
}

void idt_set_gate(uint8_t num, uint64_t base, uint16_t selector, uint8_t ist, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_mid = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;

    idt[num].selector = selector;
    idt[num].ist = ist & 0x07;
    idt[num].flags = flags;
    idt[num].reserved = 0;
}