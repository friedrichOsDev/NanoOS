/*
 * @file gdt.c
 * @brief Global Descriptor Table (GDT)
 * @author friedrichOsDev
 */

#include <gdt.h>
#include <serial.h>

/*
 * External assembly function to load the new GDT
 * @param gdt_ptr The address of the GDT pointer structure
 */
extern void gdt_flush(uint32_t gdt_ptr);

struct gdt_entry gdt[3];
struct gdt_ptr gdtp;

/*
 * Initialize the GDT with the necessary segments and load it into the CPU
 * @param void
 */
void gdt_init(void) {
    serial_printf("GDT: start\n");

    gdtp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gdtp.base = (uint32_t)&gdt;

    // Null segment
    gdt_set_gate(0, 0, 0, 0, 0);
    serial_printf("GDT: null segment set\n");
    // Code segment: base 0, limit 4GB, 32-bit, ring 0
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    serial_printf("GDT: code segment set\n");
    // Data segment: base 0, limit 4GB, 32-bit, ring 0
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    serial_printf("GDT: data segment set\n");

    serial_printf("GDT: flushing GDT\n");
    gdt_flush((uint32_t)&gdtp);

    serial_printf("GDT: done\n");
}

/*
 * Set a GDT entry with the specified parameters
 * @param num The index of the GDT entry to set
 * @param base The base address of the segment
 * @param limit The limit of the segment
 * @param access The access flags for the segment
 * @param gran The granularity flags for the segment
 */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}