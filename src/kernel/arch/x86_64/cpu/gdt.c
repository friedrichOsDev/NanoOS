/**
 * @file gdt.c
 * @brief Implementierung der GDT- und TSS-Einrichtung.
 * @author friedrichOsDev
 */

#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/cpu/gdt_flush.h>
#include <arch/x86_64/drivers/serial.h>
#include <core/panic.h>

/** @brief Globales GDT-Array für alle Segmente und TSS-Einträge aller Kerne */
struct gdt_entry gdt[GDT_ENTRIES];

/** @brief Pointer-Struktur für das `lgdt`-Assembly-Kommando */
struct gdt_ptr gdtp;

/** @brief Per-CPU TSS-Strukturen */
struct tss_entry tss_cores[MAX_CPUS];

/** @brief Dedizierte Stacks für Double Fault Exceptions (IST1), 16-Byte-ausgerichtet */
static uint8_t double_fault_stacks[MAX_CPUS][16384] __attribute__((aligned(16)));

/**
 * @brief Initialisiert die TSS-Struktur für einen bestimmten Prozessorkern.
 * 
 * Verifiziert die 16-Byte-Ausrichtung der Stacks (RSP0 und IST1) für SSE/FPU-Kompatibilität,
 * nullt die Struktur und setzt das Offset für die I/O-Bitmap auf die Größe des TSS (deaktiviert)[cite: 29].
 * 
 * @param cpu_id Die ID des Prozessorkerns[cite: 29].
 * @param kernel_stack Basisadresse des Kernel-Stacks[cite: 29].
 */
static void tss_init_core(size_t cpu_id, uintptr_t kernel_stack) {
    if (cpu_id >= MAX_CPUS) {
        return;
    }

    struct tss_entry *tss = &tss_cores[cpu_id];

    /* TSS-Struktur vollständig nullen */
    uint8_t *ptr = (uint8_t *)tss;
    for (size_t i = 0; i < sizeof(struct tss_entry); i++) {
        ptr[i] = 0;
    }

    /* Setze Kernel Stack Pointer (RSP0) für Ring 3 -> Ring 0 Wechsel */
    if (kernel_stack % 16 != 0) {
        panic("GDT TSS: RSP0 (kernel_stack) not 16-byte aligned (SSE/FPU)", kernel_stack % 16);
    }
    tss->rsp0 = (uint64_t)kernel_stack;

    /* Setze IST1 Stack Pointer für den Double Fault Exception Handler */
    uintptr_t df_stack_top = (uintptr_t)&double_fault_stacks[cpu_id][sizeof(double_fault_stacks[cpu_id])];
    if (df_stack_top % 16 != 0) {
        panic("GDT TSS: IST1 (df_stack_top) not 16-byte aligned (SSE/FPU)", df_stack_top % 16);
    }
    tss->ist1 = (uint64_t)df_stack_top;

    /* I/O-Permission-Bitmap sperren (Offset = Größe der Struktur) */
    tss->iomap_base = sizeof(struct tss_entry);

    serial_printf(COM1, "TSS Core %zu: RSP0 = %p, IST1 = %p\n", cpu_id, (void *)tss->rsp0, (void *)tss->ist1);
}

void gdt_init() {
    gdtp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdtp.base = (uint64_t)&gdt;

    /* Slot 0: Null-Deskriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Slot 1: Kernel Code (Selektor 0x08) - 64-Bit */
    gdt_set_gate(1, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_USER_SEG | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_READ_WRITE, GDT_GRAN_64BIT | GDT_GRAN_4K);

    /* Slot 2: Kernel Daten (Selektor 0x10) */
    gdt_set_gate(2, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_USER_SEG | GDT_ACCESS_READ_WRITE, GDT_GRAN_4K);

    /* Slot 3: User Daten (Selektor 0x18) */
    gdt_set_gate(3, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_USER_SEG | GDT_ACCESS_READ_WRITE, GDT_GRAN_4K);

    /* Slot 4: User Code (Selektor 0x20) - 64-Bit */
    gdt_set_gate(4, 0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_USER_SEG | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_READ_WRITE, GDT_GRAN_64BIT | GDT_GRAN_4K);

    /* GDT neu laden und Segment-Register aktualisieren */
    gdt_flush((uint64_t)&gdtp);
    serial_printf(COM1, "GDT: Flushed successfully\n");

    /* Bootstrap Processor (CPU 0) initialisieren */
    extern uint8_t stack_top[];
    gdt_init_core(0, (uintptr_t)stack_top);
}

void gdt_init_core(size_t cpu_id, uintptr_t kernel_stack) {
    if (cpu_id >= MAX_CPUS) {
        serial_printf(COM1, "GDT Core %zu: OOB cpu_id!\n", cpu_id);
        return;
    }

    /* 1. TSS-Struktur für den Kern einrichten */
    tss_init_core(cpu_id, kernel_stack);

    /* 2. TSS-Deskriptor in der GDT registrieren (belegt 2 aufeinanderfolgende Slots) */
    int gdt_index = GDT_FIRST_TSS_INDEX + (cpu_id * 2);
    gdt_set_tss_gate(gdt_index, (uintptr_t)&tss_cores[cpu_id], sizeof(struct tss_entry) - 1);

    /* 3. Task Register (TR) mit `ltr` laden */
    uint16_t tss_selector = gdt_index * 8;
    tss_load(tss_selector);

    serial_printf(COM1, "GDT Core %zu: Loaded TSS Selector %02x (Index %d)\n", cpu_id, tss_selector, gdt_index);
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

    /* Unterer 8-Byte Eintrag (Standard-System-Deskriptor) */
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].access = 0x89; /* Present | System | TSS (Available) */
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].base_high = (base >> 24) & 0xFF;

    /* Oberer 8-Byte Eintrag (Obere 32 Bits der Basisadresse) */
    struct gdt_tss_entry_high *high = (struct gdt_tss_entry_high *)&gdt[num + 1];
    high->base_upper = (uint32_t)(base >> 32);
    high->reserved = 0;
}