/**
 * @file gdt.c
 * @brief 64-bit global descriptor table setup
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/drivers/serial.h>

struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gdtp;

void gdt_init() {
    gdtp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdtp.base = (uint64_t)&gdt;

    serial_printf(COM1, "GDT: set null segment descriptor\n");
    gdt_set_gate(0, 0, 0, 0, 0);

    serial_printf(COM1, "GDT: set kernel code segment descriptor\n");
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0x20);

    serial_printf(COM1, "GDT: set user code segment descriptor\n");
    gdt_set_gate(4, 0, 0xFFFFF, 0xFA, 0x20);

    serial_printf(COM1, "GDT: set kernel data segment descriptor\n");
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0x00);

    serial_printf(COM1, "GDT: set user data segment descriptor\n");
    gdt_set_gate(3, 0, 0xFFFFF, 0xF2, 0x00);

    // TODO: tss segment

    serial_printf(COM1, "GDT: flush gdt\n");
    gdt_flush((uint64_t)&gdtp);
}

/**
 * Sets a GDT gate.
 * @param num The index of the GDT entry.
 * @param base The base address of the segment.
 * @param limit The limit of the segment.
 * @param access The access flags.
 * @param gran The granularity flags.
 */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}
