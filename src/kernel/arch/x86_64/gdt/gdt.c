/**
 * @file gdt.c
 * @brief GDT (Global Descriptor Table) implementation.
 * @author friedrichOsDev
 */

#include "gdt.h"
#include "gdt_sym.h"
#include <arch/arch.h>
#include <kernel/panic.h>

struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gdtp;
struct tss_entry tss_cores[MAX_CPUS];
static uint8_t double_fault_stacks[MAX_CPUS][16384] __attribute__((aligned(16)));

/**
 * @brief Initializes the Task State Segment (TSS) structure for a specific CPU core.
 *
 * @param[in] cpu_id Target CPU core identifier index.
 * @param[in] kernel_stack Virtual address of the top of the initial kernel stack.
 */
static void tss_init_core(size_t cpu_id, uintptr_t kernel_stack) {
    if (cpu_id >= MAX_CPUS) {
        return;
    }

    k_assert(kernel_stack % 16 == 0);

    struct tss_entry *tss = &tss_cores[cpu_id];

    uint8_t *ptr = (uint8_t *)tss;
    for (size_t i = 0; i < sizeof(struct tss_entry); i++) {
        ptr[i] = 0;
    }

    tss->rsp0 = (uint64_t)kernel_stack;

    uintptr_t df_stack_top = (uintptr_t)&double_fault_stacks[cpu_id][sizeof(double_fault_stacks[cpu_id])];
    k_assert(df_stack_top % 16 == 0);

    tss->ist1 = (uint64_t)df_stack_top;
    tss->iomap_base = (uint16_t)sizeof(struct tss_entry);

    log_debug("TSS core %zu: RSP0 = %p, IST1 = %p", cpu_id, (void *)tss->rsp0, (void *)tss->ist1);
}

/** @brief Initializes the global GDT table descriptors, flushes the GDT, and sets up BSP core 0. */
void gdt_init() {
    gdtp.limit = (uint16_t)((sizeof(struct gdt_entry) * GDT_ENTRIES) - 1);
    gdtp.base = (uint64_t)&gdt;

    // Slot 0: Null-Deskriptor
    gdt_set_gate(0, 0, 0, 0, 0);

    // Slot 1: Kernel Code (Selektor 0x08) - 64-bit
    gdt_set_gate(1, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_USER_SEG | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_READ_WRITE, GDT_GRAN_64BIT | GDT_GRAN_4K);

    // Slot 2: Kernel Daten (Selektor 0x10)
    gdt_set_gate(2, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_USER_SEG | GDT_ACCESS_READ_WRITE, GDT_GRAN_4K);

    // Slot 3: User Daten (Selektor 0x18)
    gdt_set_gate(3, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_USER_SEG | GDT_ACCESS_READ_WRITE, GDT_GRAN_4K);

    // Slot 4: User Code (Selektor 0x20) - 64-bit
    gdt_set_gate(4, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_USER_SEG | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_READ_WRITE, GDT_GRAN_64BIT | GDT_GRAN_4K);

    gdt_flush((uint64_t)&gdtp);

    gdt_init_core(0, (uintptr_t)kernel_stack_top);
}

/**
 * @brief Sets up GDT entries, TSS, and loads the task register for a specific CPU core.
 *
 * @param[in] cpu_id Target CPU core identifier index.
 * @param[in] kernel_stack Virtual address of the top of the kernel stack.
 */
void gdt_init_core(size_t cpu_id, uintptr_t kernel_stack) {
    if (cpu_id >= MAX_CPUS) {
        return;
    }

    tss_init_core(cpu_id, kernel_stack);

    size_t gdt_index = (size_t)GDT_FIRST_TSS_INDEX + (cpu_id * 2);
    gdt_set_tss_gate((int)gdt_index, (uintptr_t)&tss_cores[cpu_id], (uint32_t)(sizeof(struct tss_entry) - 1));

    uint16_t tss_selector = (uint16_t)(gdt_index * 8);
    tss_load(tss_selector);

    log_debug("GDT core %zu: Loaded TSS Selector %02x (Index %zu)", cpu_id, tss_selector, gdt_index);
}

/**
 * @brief Configures a standard GDT descriptor entry (Code/Data/Null).
 *
 * @param[in] num GDT entry index.
 * @param[in] base Base memory address.
 * @param[in] limit Segment limit.
 * @param[in] access Access permission flags.
 * @param[in] gran Granularity flags.
 */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    if (num < 0 || num >= GDT_ENTRIES) {
        return;
    }

    gdt[num].base_low = (uint16_t)(base & 0xFFFF);
    gdt[num].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt[num].base_high = (uint8_t)((base >> 24) & 0xFF);

    gdt[num].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt[num].granularity = (uint8_t)(((limit >> 16) & 0x0F) | (gran & 0xF0));
    gdt[num].access = access;
}

/**
 * @brief Configures a 16-byte system segment descriptor for the Task State Segment (TSS) spanning two GDT slots in 64-bit mode.
 *
 * @param[in] num Base GDT entry index for the TSS.
 * @param[in] base 64-bit virtual memory base address of the TSS.
 * @param[in] limit Segment limit.
 */
void gdt_set_tss_gate(int num, uintptr_t base, uint32_t limit) {
    if (num < 0 || num + 1 >= GDT_ENTRIES) {
        return;
    }

    gdt[num].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt[num].base_low = (uint16_t)(base & 0xFFFF);
    gdt[num].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt[num].access = 0x89; /* Present | System | TSS (Available) */
    gdt[num].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt[num].base_high = (uint8_t)((base >> 24) & 0xFF);

    struct gdt_tss_entry_high *high = (struct gdt_tss_entry_high *)&gdt[num + 1];
    high->base_upper = (uint32_t)(base >> 32);
    high->reserved = 0;
}

/**
 * @brief Updates the kernel stack pointer (RSP0) in the TSS for a specific CPU core.
 *
 * @param[in] cpu_id Target CPU core identifier index.
 * @param[in] stack_top New top address of the kernel stack.
 */
void gdt_set_tss_kernel_stack(size_t cpu_id, uint64_t stack_top) {
    tss_cores[cpu_id].rsp0 = stack_top;
}
