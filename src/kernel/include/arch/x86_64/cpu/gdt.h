/**
 * @file gdt.h
 * @brief Schnittstelle für die Global Descriptor Table (GDT) und das Task State Segment (TSS).
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/cpu/smp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* GDT Segment Selektoren */
#define GDT_SEL_NULL 0x00        /**< Null-Deskriptor */
#define GDT_SEL_KERN_CODE 0x08   /**< Kernel Code-Segment (Ring 0, 64-Bit) */
#define GDT_SEL_KERN_DATA 0x10   /**< Kernel Daten-Segment (Ring 0) */
#define GDT_SEL_USER_DATA 0x18   /**< User Daten-Segment (Ring 3) */
#define GDT_SEL_USER_CODE 0x20   /**< User Code-Segment (Ring 3, 64-Bit) */
#define GDT_FIRST_TSS_INDEX 5    /**< Startindex der TSS-Einträge in der GDT */

/* Zugriffsberechtigungs-Flags (Access Byte) */
#define GDT_ACCESS_PRESENT (1 << 7)     /**< Segment ist im Speicher vorhanden */
#define GDT_ACCESS_RING0 (0 << 5)       /**< Privilege Level 0 (Kernel) */
#define GDT_ACCESS_RING3 (3 << 5)       /**< Privilege Level 3 (User) */
#define GDT_ACCESS_SYSTEM (0 << 4)      /**< System-Deskriptor (z. B. TSS) */
#define GDT_ACCESS_USER_SEG (1 << 4)    /**< Code/Daten-Segment-Deskriptor */
#define GDT_ACCESS_EXECUTABLE (1 << 3)  /**< Ausführbares Code-Segment */
#define GDT_ACCESS_READ_WRITE (1 << 1)  /**< Lesbar (Code) / Schreibbar (Daten) */

/* Granularitäts-Flags */
#define GDT_GRAN_64BIT (1 << 5)  /**< Long Mode (64-Bit Code-Segment) */
#define GDT_GRAN_4K (1 << 7)     /**< Granularität in 4-KiB-Blöcken */

/** @brief Gesamtzahl der GDT-Einträge: Null, KCode, KData, UData, UCode + 2 Slots pro TSS/CPU */
#define GDT_ENTRIES (5 + (MAX_CPUS * 2))

/**
 * @brief Standard 8-Byte GDT-Eintrag für Code-/Daten-Segmente.
 */
struct gdt_entry {
    uint16_t limit_low;   /**< Untere 16 Bits des Segment-Limits */
    uint16_t base_low;    /**< Untere 16 Bits der Basisadresse */
    uint8_t base_middle;  /**< Mittlere 8 Bits der Basisadresse (Bits 16-23) */
    uint8_t access;       /**< Access-Byte (Berechtigungen und Typ) */
    uint8_t granularity;  /**< Granularitäts-Flags und obere 4 Bits des Limits */
    uint8_t base_high;    /**< Obere 8 Bits der Basisadresse (Bits 24-31) */
} __attribute__((packed));

/**
 * @brief Erweitertes System-Deskriptor-Segment (oberer Teil für 16-Byte-TSS im 64-Bit Long Mode).
 */
struct gdt_tss_entry_high {
    uint32_t base_upper;  /**< Obere 32 Bits der Basisadresse (Bits 32-63) */
    uint32_t reserved;    /**< Reserviert (muss 0 sein) */
} __attribute__((packed));

/**
 * @brief Struktur für den GDTR-Pointer (GDT-Register-Inhalt).
 */
struct gdt_ptr {
    uint16_t limit;  /**< Größe der GDT in Bytes minus 1 */
    uint64_t base;   /**< Lineare 64-Bit-Basisadresse der GDT */
} __attribute__((packed));

/**
 * @brief 64-Bit Task State Segment (TSS) Struktur.
 */
struct tss_entry {
    uint32_t reserved0;   /**< Reserviert */
    uint64_t rsp0;        /**< Kernel-Stackpointer für Privilegienwechsel nach Ring 0 */
    uint64_t rsp1;        /**< Reservierter Ring 1 Stackpointer */
    uint64_t rsp2;        /**< Reservierter Ring 2 Stackpointer */
    uint64_t reserved1;   /**< Reserviert */
    uint64_t ist1;        /**< Interrupt Stack Table Eintrag 1 (z. B. Double Fault Stack) */
    uint64_t ist2;        /**< Interrupt Stack Table Eintrag 2 */
    uint64_t ist3;        /**< Interrupt Stack Table Eintrag 3 */
    uint64_t ist4;        /**< Interrupt Stack Table Eintrag 4 */
    uint64_t ist5;        /**< Interrupt Stack Table Eintrag 5 */
    uint64_t ist6;        /**< Interrupt Stack Table Eintrag 6 */
    uint64_t ist7;        /**< Interrupt Stack Table Eintrag 7 */
    uint64_t reserved2;   /**< Reserviert */
    uint16_t reserved3;   /**< Reserviert */
    uint16_t iomap_base;  /**< Offset zur I/O-Berechtigungs-Bitmap ab Beginn des TSS */
} __attribute__((packed));

/** @brief Array der TSS-Strukturen für jeden Prozessorkern */
extern struct tss_entry tss_cores[MAX_CPUS];

/**
 * @brief Globales Initialisieren der GDT und des BSP-Kerns.
 */
void gdt_init();

/**
 * @brief Initialisiert die TSS- und GDT-Einträge für einen spezifischen Prozessorkern.
 * @param cpu_id Die ID des Prozessorkerns.
 * @param kernel_stack Basisadresse/Top des Kernel-Stacks für diesen Kern.
 */
void gdt_init_core(size_t cpu_id, uintptr_t kernel_stack);

/**
 * @brief Konfiguriert einen Standard-8-Byte-GDT-Eintrag (Gate).
 * @param num Index in der GDT.
 * @param base 32-Bit Basisadresse.
 * @param limit 20-Bit Segment-Limit.
 * @param access Access-Byte (Zugriffsrechte).
 * @param gran Granularitäts-Flags.
 */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

/**
 * @brief Konfiguriert ein 16-Byte-TSS-Gate in der GDT für den 64-Bit Long Mode.
 * @param num Index des ersten Eintrags (belegt zwei aufeinanderfolgende Slots).
 * @param base 64-Bit Basisadresse der TSS-Struktur.
 * @param limit Größe der TSS-Struktur minus 1.
 */
void gdt_set_tss_gate(int num, uintptr_t base, uint32_t limit);