/**
 * @file gdt.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/** @brief Structure representing a GDT entry. */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

/** @brief Structure representing the GDT pointer for the LGDT instruction. */
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern void gdt_flush(uint32_t gdt_ptr);

void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);