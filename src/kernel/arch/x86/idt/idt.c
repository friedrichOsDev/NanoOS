/**
 * @file idt.c
 * @author friedrichOsDev
 */

#include <idt.h>
#include <serial.h>
#include <kernel.h>
#include <interrupts.h>

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

/**
 * @brief Initializes the Interrupt Descriptor Table (IDT).
 * 
 * Sets up the IDT pointer, populates the first 32 entries with ISR stubs,
 * and loads the IDT into the CPU.
 */
void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;

    serial_printf("IDT: setting up ISR gates\n");

    void *isr_table[] = {
        isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
        isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };

    for (int i = 0; i < IDT_ENTRIES; i++) {
        if (i < 32) {
            idt_set_gate(i, (uint32_t)isr_table[i], 0x08, 0x8E);
        } else {
            idt_set_gate(i, 0, 0, 0);
        }
    }

    idt_load((uint32_t)&idtp);
    init_state = INIT_IDT;
}

/**
 * @brief Sets an IDT gate.
 * 
 * @param num The index of the IDT entry.
 * @param base The address of the interrupt handler.
 * @param sel The kernel segment selector.
 * @param flags The gate flags (type and attributes).
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;

    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].flags = flags;
}
