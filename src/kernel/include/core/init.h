/**
 * @file init.h
 * @brief Schnittstellen und Datenstrukturen für die Kernel-Initialisierung.
 * @details Definiert Symbole des Kernel-Linkers, Typen zur Multiboot2-Speicherkarte,
 *          Framebuffer-Informationen sowie Boot-Module.
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/boot/multiboot2.h>
#include <stdint.h>

extern uint8_t kernel_start[];
extern uint8_t kernel_end[];
extern uint8_t kernel_start_phys[];
extern uint8_t kernel_end_phys[];

#define KERNEL_START (uintptr_t) kernel_start
#define KERNEL_END (uintptr_t) kernel_end
#define KERNEL_START_PHYS (uintptr_t) kernel_start_phys
#define KERNEL_END_PHYS (uintptr_t) kernel_end_phys

#define MMAP_MAX_ENTRIES 1024
#define MAX_MODULES 16

/**
 * @struct fb_info_t
 * @brief Informationen zum linearen Framebuffer des Grafikmodus.
 */
typedef struct {
    uint64_t fb_addr;   /**< Physikalische Basisadresse des Framebuffers */
    uint64_t fb_width;  /**< Bildbreite in Pixeln */
    uint64_t fb_height; /**< Bildhöhe in Pixeln */
    uint64_t fb_pitch;  /**< Byte-Anzahl pro Zeile (Pitch) */
    uint64_t fb_bpp;    /**< Farbtiefe in Bits pro Pixel (BPP) */
} fb_info_t;

/**
 * @enum mmap_type_t
 * @brief Speicherbereichstypen gemäß Multiboot2-Spezifikation.
 */
typedef enum {
    MMAP_USABLE = 1,           /**< Freier, nutzbarer RAM */
    MMAP_RESERVED = 2,         /**< Reservierter Speicherbereich */
    MMAP_ACPI_RECLAIMABLE = 3, /**< ACPI-Tabellenbereich (wiederverwendbar) */
    MMAP_NVS = 4,              /**< Nicht-flüchtiger ACPI-NVS-Speicher */
    MMAP_BADRAM = 5            /**< Defekter Speicherbereich */
} mmap_type_t;

/**
 * @struct mmap_entry_t
 * @brief Einzeleintrag der Speichermap.
 */
typedef struct {
    uint64_t base_addr; /**< Physikalische Startadresse */
    uint64_t length;    /**< Länge des Speicherbereichs in Bytes */
    mmap_type_t type;   /**< Typ des Speicherbereichs */
} mmap_entry_t;

/**
 * @struct mmap_t
 * @brief Globale Speichermap-Struktur des Kernels.
 */
typedef struct {
    uint64_t entry_count;                   /**< Anzahl erfasster Einträge */
    mmap_entry_t entries[MMAP_MAX_ENTRIES]; /**< Array aller Speichereinträge */
} mmap_t;

/**
 * @struct boot_module_t
 * @brief Beschreibung eines vom Bootloader geladenen Moduls.
 */
typedef struct {
    uint32_t mod_start; /**< Physikalische Startadresse des Moduls */
    uint32_t mod_end;   /**< Physikalische Endadresse des Moduls */
    char cmdline[3];    /**< Befehlszeilenparameter des Moduls */
} boot_module_t;

/**
 * @struct boot_modules_t
 * @brief Liste aller vom Bootloader übergebenen Module.
 */
typedef struct {
    uint32_t count;                     /**< Anzahl geladener Module */
    boot_module_t entries[MAX_MODULES]; /**< Modul-Einträge */
} boot_modules_t;

extern mmap_t kernel_mmap;
extern fb_info_t kernel_fb_info;
extern multiboot_info_t *kernel_multiboot_info;
extern char kernel_cmdline[256];
extern char kernel_bootloader_name[64];
extern boot_modules_t kernel_modules;

/**
 * @brief Haupt-Einstiegspunkt des Kernels nach dem Bootloader-Übergang.
 * @param magic Multiboot2 Magic Number (muss 0x36d76289 entsprechen).
 * @param info_ptr Physikalische Adresse der Multiboot2 Informationsstruktur.
 */
void kernel_init(uint64_t magic, uint64_t info_ptr);