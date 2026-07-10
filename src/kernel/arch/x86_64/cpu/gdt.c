/**
 * @file gdt.c
 * @brief 64-bit global descriptor table setup
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/cpu/tss.h>
#include <arch/x86_64/drivers/serial.h>

struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gdtp;

void gdt_init() {
    gdtp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdtp.base = (uint64_t)&gdt;

    serial_printf(COM1, "GDT: set null segment descriptor\n");
    gdt_set_gate(0, 0, 0, 0, 0);

    serial_printf(COM1, "GDT: set kernel code descriptor\n");
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0x20);

    serial_printf(COM1, "GDT: set user code descriptor\n");
    gdt_set_gate(4, 0, 0xFFFFF, 0xFA, 0x20);

    serial_printf(COM1, "GDT: set kernel data descriptor\n");
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0x00);

    serial_printf(COM1, "GDT: set user data descriptor\n");
    gdt_set_gate(3, 0, 0xFFFFF, 0xF2, 0x00);

    tss_init();

    serial_printf(COM1, "GDT: set TSS descriptor\n");
    gdt_set_tss_gate(5, (uint64_t)&tss, sizeof(struct tss_entry) - 1);

    serial_printf(COM1, "GDT: flush GDT\n");
    gdt_flush((uint64_t)&gdtp);

    serial_printf(COM1, "GDT: load TSS\n");
    tss_load(0x28);
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

/**
 * Sets a TSS GDT descriptor (16 bytes, spanning two 8-byte slots).
 * @param num The starting index of the GDT entry.
 * @param base The 64-bit base address of the TSS.
 * @param limit The limit of the TSS.
 */
void gdt_set_tss_gate(int num, uint64_t base, uint32_t limit) {
    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].access = 0x89;
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].base_high = (base >> 24) & 0xFF;

    struct gdt_entry *high = &gdt[num + 1];
    uint32_t *base_upper = (uint32_t *)high;
    *base_upper = (base >> 32) & 0xFFFFFFFF;
    uint32_t *reserved = (uint32_t *)((uint8_t *)high + 4);
    *reserved = 0;
}
