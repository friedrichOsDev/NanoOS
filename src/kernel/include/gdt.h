/*
 * @file gdt.h
 * @brief Header file for Global Descriptor Table (GDT)
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/*
 * GDT entry structure
 */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

/*
 * GDT pointer structure
 */
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);