/**
 * @file gdt.h
 * @brief 64-bit Global Descriptor Table Setup (Header)
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/cpu/smp.h>
#include <stdint.h>

#define GDT_ENTRIES (5 + (MAX_CPUS * 2))

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern void gdt_flush(uint64_t gdt_ptr);
extern void tss_load(uint16_t selector);

void gdt_init_core(size_t cpu_id, uint64_t kernel_stack);
void gdt_init();
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access,
                  uint8_t gran);
void gdt_set_tss_gate(int num, uint64_t base, uint32_t limit);