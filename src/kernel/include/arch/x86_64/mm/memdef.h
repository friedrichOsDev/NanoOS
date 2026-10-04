/**
 * @file memdef.h
 * @brief Definitionen und Strukturen für die Speicherverwaltung (PMM, VMM und Heap).
 * @author friedrichOsDev
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/* =========================================================================
 * Allgemeine Page-Definitionen
 * ========================================================================= */

/** @brief Standard-Seitengröße (4 KiB). */
#define PAGE_SIZE 0x1000

/** @brief Bitmaske zum Isolieren der Seitenadresse. */
#define PAGE_MASK 0xFFFFFFFFFFFFF000ULL

/** @brief Typdefinition für physische Speicheradressen. */
typedef uint64_t phys_addr_t;

/** @brief Typdefinition für virtuelle Speicheradressen. */
typedef uint64_t virt_addr_t;

/* =========================================================================
 * Alignment-Hilfsmakros
 * ========================================================================= */

/**
 * @brief Prüft, ob eine Adresse an einer 4-KiB-Seitengrenze ausgerichtet ist.
 * @param addr Die zu prüfende Adresse.
 */
#define IS_PAGE_ALIGNED(addr) (((uint64_t)(addr) & 0xFFF) == 0)

/**
 * @brief Rundet eine Adresse auf die nächstgelegene untere Seitengrenze ab.
 * @param addr Die abzurundende Adresse.
 */
#define ALIGN_DOWN(addr) ((uint64_t)(addr) & PAGE_MASK)

/**
 * @brief Rundet eine Adresse auf die nächstgelegene obere Seitengrenze auf.
 * @param addr Die aufzurundende Adresse.
 */
#define ALIGN_UP(addr) (((uint64_t)(addr) + PAGE_SIZE - 1) & PAGE_MASK)

/* =========================================================================
 * Virtuelles Speicher-Layout (x86_64 Higher-Half)
 * ========================================================================= */

/** @brief Startadresse des User-Space-Bereichs. */
#define USER_SPACE_START 0x0000000001000000ULL

/** @brief Endadresse des User-Space-Bereichs. */
#define USER_SPACE_END 0x00007FFFFFFFFFFFULL

/** @brief Beginn des Kernel-Adressraums. */
#define KERNEL_SPACE_START 0xFFFF800000000000ULL

/** @brief Startadresse des Direct-Mapping-Bereichs (physischer Speicher direkt gemappt). */
#define DIRECT_MAPPING_START 0xFFFF800000000000ULL

/** @brief Endadresse des Direct-Mapping-Bereichs. */
#define DIRECT_MAPPING_END 0xFFFF807FFFFFFFFFULL

/** @brief Startadresse des dynamischen Kernel-Heaps. */
#define KERNEL_HEAP_START 0xFFFF808000000000ULL

/** @brief Endadresse des dynamischen Kernel-Heaps. */
#define KERNEL_HEAP_END 0xFFFF80FFFFFFFFFFULL

/** @brief Startadresse der Region für Memory-Mapped I/O (MMIO). */
#define MMIO_REGION_START 0xFFFF810000000000ULL

/** @brief Endadresse der Region für Memory-Mapped I/O (MMIO). */
#define MMIO_REGION_END 0xFFFFFFFF7FFFFFFFULL

/** @brief Startadresse des eigentlichen Kernel-Executable-Codes. */
#define KERNEL_CORE_START 0xFFFFFFFF80000000ULL

/**
 * @brief Konvertiert eine physische Adresse in eine virtuelle Adresse im Direct-Mapping-Bereich.
 * @param phys Physische Quelladresse.
 */
#define P2V(phys) ((virt_addr_t)(phys) + DIRECT_MAPPING_START)

/**
 * @brief Konvertiert eine virtuelle Adresse im Direct-Mapping-Bereich zurück in eine physische Adresse.
 * @param virt Virtuelle Quelladresse.
 */
#define V2P(virt) ((phys_addr_t)(virt) - DIRECT_MAPPING_START)

/* =========================================================================
 * Heap-Definitionen
 * ========================================================================= */

/** @brief Magic-Value für einen freien Heap-Block. */
#define HEAP_MAGIC_FREE 0xDEADBEEF

/** @brief Magic-Value für einen belegten Heap-Block. */
#define HEAP_MAGIC_USED 0xCAFEBABE

/** @brief Größe des Heap-Block-Headers in Bytes. */
#define HEAP_HEADER_SIZE sizeof(heap_list_t)

/** @brief Minimale Nutzlastgröße eines Heap-Blocks in Bytes. */
#define HEAP_MIN_PAYLOAD_SIZE 16

/**
 * @struct heap_list
 * @brief Header-Struktur für verkettete Heap-Speicherblöcke.
 */
typedef struct heap_list {
    uint32_t magic;         /**< Status-Magie (HEAP_MAGIC_FREE oder HEAP_MAGIC_USED). */
    uint32_t _reserved;     /**< Reserviertes Feld zur Ausrichtung. */
    uint64_t _align_pad;    /**< Alignment-Padding für 64-Bit-Grenzwerte. */
    size_t size;            /**< Gesamtgröße des Blocks (Header + Payload). */
    size_t payload_size;    /**< Reine Nutzlastgröße des Blocks. */
    struct heap_list *prev; /**< Zeiger auf den vorherigen Heap-Block. */
    struct heap_list *next; /**< Zeiger auf den nächsten Heap-Block. */
} heap_list_t;

/* =========================================================================
 * Paging-Flags und Seitentabellen-Indizes (x86_64 4-Level Paging)
 * ========================================================================= */

#define PTE_PRESENT (1ULL << 0)  /**< Seite ist im Speicher vorhanden. */
#define PTE_WRITABLE (1ULL << 1) /**< Schreibzugriff erlaubt. */
#define PTE_USER (1ULL << 2)     /**< Zugriff aus dem User-Mode erlaubt. */
#define PTE_PWT (1ULL << 3)      /**< Page-level Write-Through. */
#define PTE_PCD (1ULL << 4)      /**< Page-level Cache Disable. */
#define PTE_ACCESSED (1ULL << 5) /**< Auf Seite wurde zugegriffen. */
#define PTE_DIRTY (1ULL << 6)    /**< In Seite wurde geschrieben. */
#define PTE_HUGE (1ULL << 7)     /**< Huge Page (2 MiB / 1 GiB). */
#define PTE_GLOBAL (1ULL << 8)   /**< Globale Seite (wird bei CR3-Wechsel nicht im TLB geleert). */
#define PTE_NX (1ULL << 63)      /**< No-Execute (Ausführung von Code verhindern). */

/** @brief Bitmaske zur Extraktion der physischen Basisadresse aus einem PTE. */
#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ULL

/**
 * @brief Extrahiert die physische Zieladresse aus einem Page-Table-Eintrag.
 * @param entry Wert des Seitentabelleneintrags.
 */
#define PTE_GET_ADDR(entry) ((entry) & PTE_ADDR_MASK)

/** @brief Extrahiert den PML4-Index aus einer virtuellen Adresse. */
#define VMM_PML4_INDEX(virt) (((virt) >> 39) & 0x1FF)

/** @brief Extrahiert den PDPT-Index aus einer virtuellen Adresse. */
#define VMM_PDPT_INDEX(virt) (((virt) >> 30) & 0x1FF)

/** @brief Extrahiert den PD-Index aus einer virtuellen Adresse. */
#define VMM_PD_INDEX(virt) (((virt) >> 21) & 0x1FF)

/** @brief Extrahiert den PT-Index aus einer virtuellen Adresse. */
#define VMM_PT_INDEX(virt) (((virt) >> 12) & 0x1FF)

/** @brief Maximale Anzahl an Einträgen pro Seitentabelle (512 Einträge). */
#define PT_MAX_ENTRIES 512

/** @brief Typdefinition für einen einzelnen Seitentabelleneintrag (PTE). */
typedef uint64_t page_table_entry_t;

/**
 * @struct page_table_t
 * @brief Generische x86_64-Seitentabellenstruktur für alle 4 Hierarchie-Ebenen (PML4, PDPT, PD, PT).
 */
typedef struct {
    page_table_entry_t entries[PT_MAX_ENTRIES]; /**< Array von 512 PTE-Einträgen. */
} __attribute__((aligned(PAGE_SIZE))) page_table_t;