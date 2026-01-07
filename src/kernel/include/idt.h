#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

// Structure for an IDT entry
struct idt_entry {
    uint16_t base_low;    // The lower 16 bits of the address to jump to
    uint16_t selector;    // Kernel segment selector
    uint8_t  zero;        // This must always be zero
    uint8_t  flags;       // Type and attributes
    uint16_t base_high;   // The upper 16 bits of the address to jump to
} __attribute__((packed));

// Structure for the IDT pointer
struct idt_ptr {
    uint16_t limit;       // The upper 16 bits of all selector limits
    uint32_t base;        // The address of the first idt_entry structure
} __attribute__((packed));

void idt_init();
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif // IDT_H
