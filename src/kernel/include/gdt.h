/*
 * @file gdt.h
 * @brief Header file for Global Descriptor Table (GDT) implementation
 * @author friedrichOsDev
 */

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/*
 * Structure representing a GDT entry
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
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
 * Structure representing the pointer to the GDT
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
struct gdt_ptr {
    uint16_t limit; 
    uint32_t base; 
} __attribute__((packed));

void gdt_init();
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

#endif // GDT_H