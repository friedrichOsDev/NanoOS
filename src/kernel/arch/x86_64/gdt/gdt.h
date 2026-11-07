/**
 * @file gdt.h
 * @brief GDT definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

#define GDT_SEL_NULL 0x00
#define GDT_SEL_KERN_CODE 0x08
#define GDT_SEL_KERN_DATA 0x10
#define GDT_SEL_USER_DATA 0x18
#define GDT_SEL_USER_CODE 0x20
#define GDT_FIRST_TSS_INDEX 5

#define GDT_ACCESS_PRESENT (1 << 7)
#define GDT_ACCESS_RING0 (0 << 5)
#define GDT_ACCESS_RING3 (3 << 5)
#define GDT_ACCESS_SYSTEM (0 << 4)
#define GDT_ACCESS_USER_SEG (1 << 4)
#define GDT_ACCESS_EXECUTABLE (1 << 3)
#define GDT_ACCESS_READ_WRITE (1 << 1)

#define GDT_GRAN_64BIT (1 << 5)
#define GDT_GRAN_4K (1 << 7)
#define GDT_ENTRIES (5 + (MAX_CPUS * 2))

/** @brief Standard 8-byte GDT entry for code and data segments. */
struct gdt_entry {
    uint16_t limit_low;  /**< Lower 16 bits of the segment limit */
    uint16_t base_low;   /**< Lower 16 bits of the base address */
    uint8_t base_middle; /**< Middle 8 bits of the base address (bits 16-23) */
    uint8_t access;      /**< Access byte (permissions and type) */
    uint8_t granularity; /**< Granularity flags and upper 4 bits of the limit */
    uint8_t base_high;   /**< Upper 8 bits of the base address (bits 24-31) */
} __attribute__((packed));

/** @brief Extended system descriptor segment (upper part for 16-byte TSS in 64-bit long mode). */
struct gdt_tss_entry_high {
    uint32_t base_upper; /**< Upper 32 bits of the base address (bits 32-63) */
    uint32_t reserved;   /**< Reserved (must be zero) */
} __attribute__((packed));

/** @brief Structure for the GDTR pointer (GDT register payload). */
struct gdt_ptr {
    uint16_t limit; /**< Size of the GDT in bytes minus 1 */
    uint64_t base;  /**< Linear 64-bit base address of the GDT */
} __attribute__((packed));

/** @brief 64-bit Task State Segment (TSS) structure. */
struct tss_entry {
    uint32_t reserved0;  /**< Reserved */
    uint64_t rsp0;       /**< Kernel stack pointer for privilege level transitions to ring 0 */
    uint64_t rsp1;       /**< Reserved ring 1 stack pointer */
    uint64_t rsp2;       /**< Reserved ring 2 stack pointer */
    uint64_t reserved1;  /**< Reserved */
    uint64_t ist1;       /**< Interrupt Stack Table entry 1 (e.g., double fault stack) */
    uint64_t ist2;       /**< Interrupt Stack Table entry 2 */
    uint64_t ist3;       /**< Interrupt Stack Table entry 3 */
    uint64_t ist4;       /**< Interrupt Stack Table entry 4 */
    uint64_t ist5;       /**< Interrupt Stack Table entry 5 */
    uint64_t ist6;       /**< Interrupt Stack Table entry 6 */
    uint64_t ist7;       /**< Interrupt Stack Table entry 7 */
    uint64_t reserved2;  /**< Reserved */
    uint16_t reserved3;  /**< Reserved */
    uint16_t iomap_base; /**< Offset to the I/O permission bitmap from the start of the TSS */
} __attribute__((packed));
