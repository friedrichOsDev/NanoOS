/*
 * @file idt.c
 * @brief Interrupt Descriptor Table (IDT)
 * @author friedrichOsDev
 */

#include <idt.h>
#include <serial.h>

/*
 * External assembly function to load the IDT
 * @param idt_ptr The address of the IDT pointer structure
 */
extern void idt_load(uint32_t idt_ptr);

/*
 * External assembly ISR handlers
 * @param void
 */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

/*
 * Initialize the IDT
 * @param void
 */
void idt_init(void) {
    serial_printf("IDT: start\n");

    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;

    void *isr_table[] = {
        isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
        isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };

    for (int i = 0; i < IDT_ENTRIES; i++) {
        if (i < 32) {
            idt_set_gate(i, (uint32_t)isr_table[i], 0x08, 0x8E);
            serial_printf("IDT: ISR %d set\n", i);
        } else {
            idt_set_gate(i, 0, 0, 0);
        }
    }

    idt_load((uint32_t)&idtp);

    serial_printf("IDT: done\n");
}

/*
 * Set an IDT entry with the specified parameters
 * @param num The index of the IDT entry to set
 * @param base The address of the ISR
 * @param sel The segment selector
 * @param flags The flags for the IDT entry
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;

    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

/*
 * Enable interrupts
 * @param void
 */
void idt_enable(void) {
    __asm__ __volatile__ ("sti");
}