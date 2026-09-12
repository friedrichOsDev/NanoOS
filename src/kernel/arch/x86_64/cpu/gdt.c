/**
 * @file gdt.c
 * @brief GDT & TSS Setup
 */

#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/drivers/serial.h>

/* Global GDT array & GDTR pointer */
struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gdtp;

/* Per-CPU TSS structures and dedicated Double Fault Stacks (16 KB aligned) */
static struct tss_entry tss_cores[MAX_CPUS];
static uint8_t double_fault_stacks[MAX_CPUS][16384] __attribute__((aligned(16)));

/**
 * @brief Initializes the TSS structure for a specific CPU core.
 * @param cpu_id The ID of the CPU core.
 * @param kernel_stack The address of the kernel
 * @return void
 */
static void tss_init_core(size_t cpu_id, uintptr_t kernel_stack) {
    if (cpu_id >= MAX_CPUS) {
        return;
    }

    struct tss_entry *tss = &tss_cores[cpu_id];

    /* Zero-out TSS structure */
    uint8_t *ptr = (uint8_t *)tss;
    for (size_t i = 0; i < sizeof(struct tss_entry); i++) {
        ptr[i] = 0;
    }

    /* Set Kernel Stack Pointer (RSP0) for Ring 3 -> Ring 0 transitions */
    tss->rsp0 = (uint64_t)kernel_stack;

    /* Set IST1 Stack Pointer for Double Fault Exception Handler */
    uintptr_t df_stack_top = (uintptr_t)&double_fault_stacks[cpu_id][sizeof(double_fault_stacks[cpu_id])];
    tss->ist1 = (uint64_t)df_stack_top;

    /* Offset to IO Permission Bitmap (set to size of TSS = disabled) */
    tss->iomap_base = sizeof(struct tss_entry);

    serial_printf(COM1, "TSS Core %zu: RSP0 = 0x%p, IST1 = 0x%p\n", cpu_id, (void *)tss->rsp0, (void *)tss->ist1);
}

void gdt_init(void) {
    gdtp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdtp.base = (uint64_t)&gdt;

    /* Slot 0: Null Descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Slot 1: Kernel Code (Selector 0x08) - 64-bit */
    gdt_set_gate(1, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_USER_SEG | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_READ_WRITE, GDT_GRAN_64BIT | GDT_GRAN_4K);

    /* Slot 2: Kernel Data (Selector 0x10) */
    gdt_set_gate(2, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_USER_SEG | GDT_ACCESS_READ_WRITE, GDT_GRAN_4K);

    /* Slot 3: User Data (Selector 0x18) */
    gdt_set_gate(3, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_USER_SEG | GDT_ACCESS_READ_WRITE, GDT_GRAN_4K);

    /* Slot 4: User Code (Selector 0x20) - 64-bit */
    gdt_set_gate(4, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_USER_SEG | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_READ_WRITE, GDT_GRAN_64BIT | GDT_GRAN_4K);

    /* Flush GDT and apply new Data Selectors */
    gdt_flush((uint64_t)&gdtp);
    serial_printf(COM1, "GDT: Flushed successfully\n");

    /* Initialize BSP Core (CPU 0) */
    extern uint8_t stack_top[];
    gdt_init_core(0, (uintptr_t)stack_top);
}

void gdt_init_core(size_t cpu_id, uintptr_t kernel_stack) {
    if (cpu_id >= MAX_CPUS) {
        serial_printf(COM1, "GDT Core %zu: OOB cpu_id!\n", cpu_id);
        return;
    }

    /* 1. Setup TSS Core structure */
    tss_init_core(cpu_id, kernel_stack);

    /* 2. Register TSS Descriptor in GDT (Takes 2 consecutive slots) */
    int gdt_index = GDT_FIRST_TSS_INDEX + (cpu_id * 2);
    gdt_set_tss_gate(gdt_index, (uintptr_t)&tss_cores[cpu_id], sizeof(struct tss_entry) - 1);

    /* 3. Load Task Register (TR) */
    uint16_t tss_selector = gdt_index * 8;
    tss_load(tss_selector);

    serial_printf(COM1, "GDT Core %zu: Loaded TSS Selector 0x%02X (Index %d)\n",
                  cpu_id, tss_selector, gdt_index);
}

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    if (num < 0 || num >= GDT_ENTRIES) {
        return;
    }

    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

void gdt_set_tss_gate(int num, uintptr_t base, uint32_t limit) {
    if (num < 0 || num + 1 >= GDT_ENTRIES) {
        return;
    }

    /* Low 8-byte Entry (Standard Descriptor Structure for System Gate) */
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].access = 0x89; /* Present | System | TSS (Available) */
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].base_high = (base >> 24) & 0xFF;

    /* High 8-byte Entry (Upper 32-bits of base address) */
    struct gdt_tss_entry_high *high = (struct gdt_tss_entry_high *)&gdt[num + 1];
    high->base_upper = (uint32_t)(base >> 32);
    high->reserved = 0;
}