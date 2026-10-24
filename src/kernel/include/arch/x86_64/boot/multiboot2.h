/**
 * @file multiboot2.h
 * @brief Definitionen und Datenstrukturen der Multiboot2-Spezifikation.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/** @brief Magische Zahl im EAX-Register, die einen Multiboot2-konformen Bootloader anzeigt. */
#define MULTIBOOT2_MAGIC 0x36D76289

/** @brief Tag-Typ: Ende der Multiboot2-Tag-Liste. */
#define MULTIBOOT_TAG_TYPE_END 0
/** @brief Tag-Typ: Kernel-Kommandozeile. */
#define MULTIBOOT_TAG_TYPE_CMDLINE 1
/** @brief Tag-Typ: Name des Bootloaders. */
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER 2
/** @brief Tag-Typ: Geladenes Boot-Modul. */
#define MULTIBOOT_TAG_TYPE_MODULE 3
/** @brief Tag-Typ: Basic Memory Information (unterer/oberer Speicher). */
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
/** @brief Tag-Typ: Memory Map (Speicherkarte des Systems). */
#define MULTIBOOT_TAG_TYPE_MMAP 6
/** @brief Tag-Typ: Framebuffer-Informationen. */
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
/** @brief Tag-Typ: ELF-Sektions-Header. */
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS 9
/** @brief Tag-Typ: EFI Boot Services nicht beendet. */
#define MULTIBOOT_TAG_TYPE_EFI_BS 12
/** @brief Tag-Typ: ACPI v1.0 RSDP. */
#define MULTIBOOT_TAG_TYPE_ACPI_OLD 14
/** @brief Tag-Typ: ACPI v2.0+ RSDP. */
#define MULTIBOOT_TAG_TYPE_ACPI_NEW 15
/** @brief Tag-Typ: EFI 64-Bit System Table Pointer. */
#define MULTIBOOT_TAG_TYPE_EFI_64 17
/** @brief Tag-Typ: Physische Lade-Basisadresse des Kernels. */
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR 21

/**
 * @brief Hauptheader der Multiboot2-Informationsstruktur.
 *
 * Der Pointer auf diese Struktur wird vom Bootloader im EBX-Register übergeben.
 */
typedef struct {
    uint32_t total_size; /**< Gesamtgröße der Multiboot2-Informationen inklusive aller Tags in Bytes. */
    uint32_t reserved;   /**< Reserviertes Feld (muss 0 sein). */
} __attribute__((packed)) multiboot_info_t;

/**
 * @brief Generischer Header für alle Multiboot2-Tags.
 */
typedef struct {
    uint32_t type; /**< Typ des Tags (MULTIBOOT_TAG_TYPE_*). */
    uint32_t size; /**< Größe des gesamten Tags in Bytes. */
} __attribute__((packed)) multiboot_tag_t;

/**
 * @brief Tag für die dem Kernel übergebene Kommandozeile.
 */
typedef struct {
    uint32_t type; /**< MULTIBOOT_TAG_TYPE_CMDLINE */
    uint32_t size; /**< Größe des Tags */
    char string[]; /**< Nullterminierter C-String der Kommandozeile */
} __attribute__((packed)) multiboot_tag_cmdline_t;

/**
 * @brief Tag für den Namen/die Version des ausführenden Bootloaders.
 */
typedef struct {
    uint32_t type; /**< MULTIBOOT_TAG_TYPE_BOOT_LOADER */
    uint32_t size; /**< Größe des Tags */
    char string[]; /**< Nullterminierter C-String mit dem Bootloader-Namen */
} __attribute__((packed)) multiboot_tag_boot_loader_t;

/**
 * @brief Tag für vom Bootloader mitgeladene Module (z. B. Initrd, Treiber).
 */
typedef struct {
    uint32_t type;      /**< MULTIBOOT_TAG_TYPE_MODULE */
    uint32_t size;      /**< Größe des Tags */
    uint32_t mod_start; /**< Physische Startadresse des Moduls im Speicher */
    uint32_t mod_end;   /**< Physische Endadresse des Moduls im Speicher */
    char cmdline[];     /**< Optionale Kommandozeile/Parameter für das Modul */
} __attribute__((packed)) multiboot_tag_module_t;

/**
 * @brief Einzelner Eintrag in der Multiboot2-Memory-Map.
 */
typedef struct {
    uint64_t base_addr; /**< Startadresse des Speicherbereichs */
    uint64_t length;    /**< Länge des Speicherbereichs in Bytes */
    uint32_t type;      /**< Typ des Speichers (1 = Verwendbar, sonst reserviert/NVDIMM/ACPI) */
    uint32_t reserved;  /**< Reserviert */
} __attribute__((packed)) multiboot_tag_mmap_entry_t;

/**
 * @brief Tag für die vollständige Memory Map des Systems.
 */
typedef struct {
    uint32_t type;                        /**< MULTIBOOT_TAG_TYPE_MMAP */
    uint32_t size;                        /**< Größe des Tags */
    uint32_t entry_size;                  /**< Größe eines einzelnen `multiboot_tag_mmap_entry_t` Eintrags */
    uint32_t entry_version;               /**< Version der Eintragsstruktur (derzeit 0) */
    multiboot_tag_mmap_entry_t entries[]; /**< Feld der Memory-Map-Einträge */
} __attribute__((packed)) multiboot_tag_mmap_t;

/**
 * @brief Farbkomponenten-Eintrag für palettenbasierte Framebuffer.
 */
typedef struct {
    uint8_t red;   /**< Rot-Anteil (0–255) */
    uint8_t green; /**< Grün-Anteil (0–255) */
    uint8_t blue;  /**< Blau-Anteil (0–255) */
} __attribute__((packed)) multiboot_tag_framebuffer_palette_t;

/**
 * @brief Farbinformationen für Paletten-Framebuffer.
 */
typedef struct {
    uint32_t framebuffer_palette_num_colors;                   /**< Anzahl der Farbpaletten-Einträge */
    multiboot_tag_framebuffer_palette_t framebuffer_palette[]; /**< Paletten-Array */
} __attribute__((packed)) multiboot_tag_framebuffer_color_info_t;

/**
 * @brief Layout der Bitfelder für Direct-Color/RGB-Framebuffer.
 */
typedef struct {
    uint8_t framebuffer_red_field_position;   /**< Bit-Offset der Rotkomponente */
    uint8_t framebuffer_red_mask_size;        /**< Bit-Größe der Rotkomponente */
    uint8_t framebuffer_green_field_position; /**< Bit-Offset der Grünkomponente */
    uint8_t framebuffer_green_mask_size;      /**< Bit-Größe der Grünkomponente */
    uint8_t framebuffer_blue_field_position;  /**< Bit-Offset der Blaukomponente */
    uint8_t framebuffer_blue_mask_size;       /**< Bit-Größe der Blaukomponente */
} __attribute__((packed)) multiboot_tag_framebuffer_rgb_info_t;

/**
 * @brief Tag für die Grafik-Framebuffer-Konfiguration.
 */
typedef struct {
    uint32_t type;               /**< MULTIBOOT_TAG_TYPE_FRAMEBUFFER */
    uint32_t size;               /**< Größe des Tags */
    uint64_t framebuffer_addr;   /**< Physische Basisadresse des Framebuffers */
    uint32_t framebuffer_pitch;  /**< Pitch (Bytes pro Zeile) */
    uint32_t framebuffer_width;  /**< Breite in Pixeln */
    uint32_t framebuffer_height; /**< Höhe in Pixeln */
    uint8_t framebuffer_bpp;     /**< Farbtiefe in Bits per Pixel */
    uint8_t framebuffer_type;    /**< Framebuffer-Typ (0 = Indexed/Palette, 1 = RGB, 2 = EGA-Text) */
    uint16_t reserved;           /**< Reserviert */
    union {
        struct {
            uint16_t framebuffer_palette_num_colors;                    /**< Anzahl Farben bei palettenbasiertem Modus */
            multiboot_tag_framebuffer_palette_t framebuffer_palette[1]; /**< Farbpalette */
        };
        struct {
            uint8_t framebuffer_red_field_position;   /**< Rot-Bit-Shift */
            uint8_t framebuffer_red_mask_size;        /**< Rot-Bit-Länge */
            uint8_t framebuffer_green_field_position; /**< Grün-Bit-Shift */
            uint8_t framebuffer_green_mask_size;      /**< Grün-Bit-Länge */
            uint8_t framebuffer_blue_field_position;  /**< Blau-Bit-Shift */
            uint8_t framebuffer_blue_mask_size;       /**< Blau-Bit-Länge */
        };
    };
} __attribute__((packed)) multiboot_tag_framebuffer_t;

/**
 * @brief Tag für die alte ACPI v1.0 Root System Description Pointer (RSDP) Struktur.
 */
typedef struct {
    uint32_t type;  /**< MULTIBOOT_TAG_TYPE_ACPI_OLD */
    uint32_t size;  /**< Größe des Tags */
    uint8_t rsdp[]; /**< Rohdaten der ACPI 1.0 RSDP-Struktur */
} __attribute__((packed)) multiboot_tag_old_acpi_t;

/**
 * @brief Tag für die neue ACPI v2.0+ Root System Description Pointer (RSDP) Struktur.
 */
typedef struct {
    uint32_t type;  /**< MULTIBOOT_TAG_TYPE_ACPI_NEW */
    uint32_t size;  /**< Größe des Tags */
    uint8_t rsdp[]; /**< Rohdaten der ACPI 2.0+ RSDP-Struktur */
} __attribute__((packed)) multiboot_tag_new_acpi_t;

/**
 * @brief Tag für grundlegende Angaben zum konventionellen Speicher.
 */
typedef struct {
    uint32_t type;      /**< MULTIBOOT_TAG_TYPE_BASIC_MEMINFO */
    uint32_t size;      /**< Größe des Tags */
    uint32_t mem_lower; /**< Unterer Speicherbereich in KiB (unterhalb von 640 KiB) */
    uint32_t mem_upper; /**< Oberer Speicherbereich in KiB (oberhalb von 1 MiB) */
} __attribute__((packed)) multiboot_tag_basic_meminfo_t;

/**
 * @brief Tag für die tatsächliche physische Ladeadresse des Kernels.
 */
typedef struct {
    uint32_t type;           /**< MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR */
    uint32_t size;           /**< Größe des Tags */
    uint32_t load_base_addr; /**< Physische Basisadresse */
} __attribute__((packed)) multiboot_tag_load_base_addr_t;

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

void multiboot_parse(const uint64_t magic, const uint64_t info_ptr);
phys_addr_t get_rsdp_phys_addr();
