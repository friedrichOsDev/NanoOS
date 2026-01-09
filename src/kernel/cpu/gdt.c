#include <gdt.h>
#include <serial.h>

extern void gdt_flush(uint32_t);

struct gdt_entry gdt[3];
struct gdt_ptr gp;

void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);
    serial_puts("gdt_init: set null segment\n");
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    serial_puts("gdt_init: set code segment\n");
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    serial_puts("gdt_init: set data segment\n");

    gdt_flush((uint32_t)&gp);
    serial_puts("gdt_init: flushed gdt\n");
}

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}
