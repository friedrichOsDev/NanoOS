/**
 * @file gdt.h
 * @brief GDT & TSS Setup
 */

#pragma once

#include <arch/x86_64/cpu/smp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* GDT Segment Selectors */
#define GDT_SEL_NULL 0x00
#define GDT_SEL_KERN_CODE 0x08
#define GDT_SEL_KERN_DATA 0x10
#define GDT_SEL_USER_DATA 0x18
#define GDT_SEL_USER_CODE 0x20
#define GDT_FIRST_TSS_INDEX 5

/* Access Flags */
#define GDT_ACCESS_PRESENT (1 << 7)
#define GDT_ACCESS_RING0 (0 << 5)
#define GDT_ACCESS_RING3 (3 << 5)
#define GDT_ACCESS_SYSTEM (0 << 4)
#define GDT_ACCESS_USER_SEG (1 << 4)
#define GDT_ACCESS_EXECUTABLE (1 << 3)
#define GDT_ACCESS_READ_WRITE (1 << 1)

/* Granularity Flags */
#define GDT_GRAN_64BIT (1 << 5)
#define GDT_GRAN_4K (1 << 7)

/* Total entries: Null, KCode, KData, UData, UCode + (2 slots per TSS per CPU) */
#define GDT_ENTRIES (5 + (MAX_CPUS * 2))

/**
 * @brief Standard 8-byte GDT Entry
 */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

/**
 * @brief System Segment Descriptor High-Part (for 16-byte TSS Entries in Long Mode)
 */
struct gdt_tss_entry_high {
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed));

/**
 * @brief GDT Register Pointer structure
 */
struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/**
 * @brief 64-bit Task State Segment (TSS)
 */
struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

/**
 * @brief GDT Flush function
 * @param gdt_ptr_phys The physical address of the GDT pointer
 * @return void
 */
extern void gdt_flush(uint64_t gdt_ptr_phys);

/**
 * @brief TSS Load function
 * @param selector The selector to load the TSS with
 * @return void
 */
extern void tss_load(uint16_t selector);

/**
 * @brief GDT Initialization function
 * @return void
 */
void gdt_init();

/**
 * @brief Per CPU GDT Initialization function
 * @param cpu_id The CPU ID
 * @param kernel_stack The kernel stack pointer
 * @return void
 */
void gdt_init_core(size_t cpu_id, uintptr_t kernel_stack);

/**
 * @brief Set GDT gate function
 * @param num The gate number
 * @param base The base address of the gate
 * @param limit The limit of the gate
 * @param access The access rights of the gate
 * @param gran The granularity of the gate
 * @return void
 */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

/**
 * @brief Set TSS gate function
 * @param num The gate number
 * @param base The base address of the gate
 * @param limit The limit of the gate
 * @return void
 */
void gdt_set_tss_gate(int num, uintptr_t base, uint32_t limit);