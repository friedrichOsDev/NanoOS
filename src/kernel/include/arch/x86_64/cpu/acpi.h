/**
 * @file acpi.h
 * @brief 64-Bit-ACPI-Strukturen (Advanced Configuration and Power Interface).
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/mm/memdef.h>
#include <stdint.h>

#define RSDP_SIGNATURE "RSD PTR "
#define FADT_SIGNATURE "FACP"
#define MADT_SIGNATURE "APIC"
#define HPET_SIGNATURE "HPET"
#define MCFG_SIGNATURE "MCFG"

#define MADT_LAPIC_TYPE 0
#define MADT_IOAPIC_TYPE 1
#define MADT_ISO_TYPE 2
#define MADT_IOAPIC_NMI_TYPE 3
#define MADT_LAPIC_NMI_TYPE 4
#define MADT_LAPIC_ADDRESS_OVERRIDE_TYPE 5
#define MADT_LX2APIC_TYPE 9

#define MAX_MCFG_ENTRIES 8

/**
 * @struct rsdp_t
 * @brief Root System Description Pointer (RSDP).
 */
typedef struct {
    char signature[8];         /**< Signatur "RSD PTR ". */
    uint8_t checksum;          /**< Prüfsumme der ersten 20 Bytes. */
    char oem_id[6];            /**< OEM-Kennung. */
    uint8_t revision;          /**< ACPI-Revision (0/1 = ACPI 1.0, >=2 = ACPI 2.0+). */
    uint32_t rsdt_address;     /**< Physische Adresse der 32-Bit-RSDT. */
    uint32_t length;           /**< Gesamtlänge der RSDP-Struktur. */
    uint64_t xsdt_address;     /**< Physische Adresse der 64-Bit-XSDT. */
    uint8_t extended_checksum; /**< Erweiterte Prüfsumme. */
    uint8_t reserved[3];       /**< Reservierte Bytes. */
} __attribute__((packed)) rsdp_t;

/**
 * @struct acpi_sdt_header_t
 * @brief Gemeinsamer Header für alle ACPI System Description Tables (SDT).
 */
typedef struct {
    char signature[4];         /**< Tabellensignatur (z. B. "APIC", "FACP"). */
    uint32_t length;           /**< Gesamtlänge der Tabelle in Bytes inklusive Header. */
    uint8_t revision;          /**< Tabellenrevision. */
    uint8_t checksum;          /**< Prüfsumme für die gesamte Tabelle. */
    char oem_id[6];            /**< OEM-ID. */
    char oem_table_id[8];      /**< OEM-Tabellen-ID. */
    uint32_t oem_revision;     /**< OEM-Revision. */
    uint32_t creator_id;       /**< ID des Erstellers (z. B. ASL-Compiler). */
    uint32_t creator_revision; /**< Revision des Erstellers. */
} __attribute__((packed)) acpi_sdt_header_t;

/**
 * @struct rsdt_t
 * @brief Root System Description Table (32-Bit-Zeiger).
 */
typedef struct {
    acpi_sdt_header_t header;        /**< Gemeinsamer SDT-Header. */
    uint32_t pointer_to_other_sdt[]; /**< Array von 32-Bit-Adressen anderer ACPI-Tabellen. */
} __attribute__((packed)) rsdt_t;

/**
 * @struct xsdt_t
 * @brief Extended System Description Table (64-Bit-Zeiger).
 */
typedef struct {
    acpi_sdt_header_t header;        /**< Gemeinsamer SDT-Header. */
    uint64_t pointer_to_other_sdt[]; /**< Array von 64-Bit-Adressen anderer ACPI-Tabellen. */
} __attribute__((packed)) xsdt_t;

/**
 * @struct acpi_gas_t
 * @brief Generic Address Structure (GAS).
 */
typedef struct {
    uint8_t address_space_id;    /**< Adressraum-ID (0=System Memory, 1=System I/O, etc.). */
    uint8_t register_bit_width;  /**< Registerbreite in Bits. */
    uint8_t register_bit_offset; /**< Bit-Offset innerhalb des Registers. */
    uint8_t access_size;         /**< Zugriffsgröße. */
    uint64_t address;            /**< 64-Bit-Registeradresse. */
} __attribute__((packed)) acpi_gas_t;

/**
 * @struct fadt_t
 * @brief Fixed ACPI Description Table (FADT).
 */
typedef struct {
    acpi_sdt_header_t header;     /**< Gemeinsamer SDT-Header. */
    uint32_t firmware_ctrl;       /**< Physische Adresse der FACS. */
    uint32_t dsdt;                /**< 32-Bit-Adresse der DSDT. */
    uint8_t reserved;             /**< Reserviert. */
    uint8_t preferred_pm_profile; /**< Bevorzugtes Power-Management-Profil. */
    uint16_t sci_int;             /**< SCI-Systeminterrupt. */
    uint32_t smi_cmd;             /**< SMI-Befehlsport. */
    uint8_t acpi_enable;          /**< Wert zum Aktivieren von ACPI. */
    uint8_t acpi_disable;         /**< Wert zum Deaktivieren von ACPI. */
    uint8_t s4bios_req;           /**< S4BIOS-Anforderungswert. */
    uint8_t pstate_cnt;           /**< P-State-Steuerwert. */
    uint32_t pm1a_evt_blk;        /**< PM1a-Event-Blockadresse. */
    uint32_t pm1b_evt_blk;        /**< PM1b-Event-Blockadresse. */
    uint32_t pm1a_cnt_blk;        /**< PM1a-Control-Blockadresse. */
    uint32_t pm1b_cnt_blk;        /**< PM1b-Control-Blockadresse. */
    uint32_t pm2_cnt_blk;         /**< PM2-Control-Blockadresse. */
    uint32_t pm_tmr_blk;          /**< PM-Timer-Blockadresse. */
    uint32_t gpe0_blk;            /**< GPE0-Blockadresse. */
    uint32_t gpe1_blk;            /**< GPE1-Blockadresse. */
    uint8_t pm1_evt_len;          /**< Länge des PM1-Event-Blocks. */
    uint8_t pm1_cnt_len;          /**< Länge des PM1-Control-Blocks. */
    uint8_t pm2_cnt_len;          /**< Länge des PM2-Control-Blocks. */
    uint8_t pm_tmr_len;           /**< Länge des PM-Timers. */
    uint8_t gpe0_blk_len;         /**< Länge des GPE0-Blocks. */
    uint8_t gpe1_blk_len;         /**< Länge des GPE1-Blocks. */
    uint8_t gpe1_base;            /**< GPE1-Basisoffset. */
    uint8_t cst_cnt;              /**< C-State-Steuerwert. */
    uint16_t p_lvl2_lat;          /**< C2-Latenz. */
    uint16_t p_lvl3_lat;          /**< C3-Latenz. */
    uint16_t flush_size;          /**< Flush-Größe. */
    uint16_t flush_stride;        /**< Flush-Schrittweite. */
    uint8_t duty_offset;          /**< Duty-Cycle-Offset. */
    uint8_t duty_width;           /**< Duty-Cycle-Breite. */
    uint8_t day_alrm;             /**< RTC-Tagesalarm-Index. */
    uint8_t mon_alrm;             /**< RTC-Monatsalarm-Index. */
    uint8_t century;              /**< RTC-Jahrhundert-Index. */
    uint16_t iapc_boot_arch;      /**< IA-PC-Boot-Architektur-Flags. */
    uint8_t reserved2;            /**< Reserviert. */
    uint32_t flags;               /**< Allgemeine ACPI-Feature-Flags. */
    acpi_gas_t reset_reg;         /**< Reset-Register-Adresse. */
    uint8_t reset_value;          /**< Reset-Befehlswert. */
    uint8_t reserved3[3];         /**< Reserviert. */
    uint64_t x_firmware_ctrl;     /**< 64-Bit-Adresse der FACS. */
    uint64_t x_dsdt;              /**< 64-Bit-Adresse der DSDT. */
    acpi_gas_t x_pm1a_evt_blk;    /**< 64-Bit-PM1a-Event-Block. */
    acpi_gas_t x_pm1b_evt_blk;    /**< 64-Bit-PM1b-Event-Block. */
    acpi_gas_t x_pm1a_cnt_blk;    /**< 64-Bit-PM1a-Control-Block. */
    acpi_gas_t x_pm1b_cnt_blk;    /**< 64-Bit-PM1b-Control-Block. */
    acpi_gas_t x_pm2_cnt_blk;     /**< 64-Bit-PM2-Control-Block. */
    acpi_gas_t x_pm_tmr_blk;      /**< 64-Bit-PM-Timer-Block. */
    acpi_gas_t x_gpe0_blk;        /**< 64-Bit-GPE0-Block. */
    acpi_gas_t x_gpe1_blk;        /**< 64-Bit-GPE1-Block. */
} __attribute__((packed)) fadt_t;

/**
 * @struct madt_t
 * @brief Multiple APIC Description Table (MADT).
 */
typedef struct {
    acpi_sdt_header_t header;    /**< Gemeinsamer SDT-Header. */
    uint32_t local_apic_address; /**< Physische Basisadresse des Local APIC. */
    uint32_t flags;              /**< APIC-Flags (z. B. Dual PIC enthalten). */
    uint8_t entries[];           /**< Variable Liste von MADT-Einträgen. */
} __attribute__((packed)) madt_t;

/** @brief Header für Unterstrukturen innerhalb der MADT. */
typedef struct {
    uint8_t type;   /**< Typ des MADT-Eintrags. */
    uint8_t length; /**< Länge des Eintrags in Bytes. */
} __attribute__((packed)) madt_entry_header_t;

/** @brief Local APIC Struktur in der MADT (Typ 0). */
typedef struct {
    madt_entry_header_t header; /**< MADT-Eintragsheader. */
    uint8_t acpi_processor_id;  /**< ACPI-Prozessor-ID. */
    uint8_t apic_id;            /**< Local APIC ID. */
    uint32_t flags;             /**< Statusflags (z. B. Enabled). */
} __attribute__((packed)) madt_lapic_entry_t;

/** @brief I/O APIC Struktur in der MADT (Typ 1). */
typedef struct {
    madt_entry_header_t header;            /**< MADT-Eintragsheader. */
    uint8_t ioapic_id;                     /**< I/O APIC ID. */
    uint8_t reserved;                      /**< Reserviert. */
    uint32_t ioapic_address;               /**< Physische MMIO-Adresse des I/O APIC. */
    uint32_t global_system_interrupt_base; /**< Basis der Global System Interrupts (GSI). */
} __attribute__((packed)) madt_ioapic_entry_t;

/** @brief Interrupt Source Override Struktur in der MADT (Typ 2). */
typedef struct {
    madt_entry_header_t header;       /**< MADT-Eintragsheader. */
    uint8_t bus;                      /**< Bus-Typ (stets 0 = ISA). */
    uint8_t source;                   /**< ISA IRQ-Quelle. */
    uint32_t global_system_interrupt; /**< Zielleitung im GSI-Raum. */
    uint16_t flags;                   /**< Polarität und Trigger-Modus. */
} __attribute__((packed)) madt_iso_entry_t;

/** @brief I/O APIC NMI Source Struktur in der MADT (Typ 3). */
typedef struct {
    madt_entry_header_t header;       /**< MADT-Eintragsheader. */
    uint8_t ioapic_id;                /**< I/O APIC ID. */
    uint8_t reserved;                 /**< Reserviert. */
    uint32_t global_system_interrupt; /**< GSI-Nummer für NMI. */
} __attribute__((packed)) madt_ioapic_nmi_entry_t;

/** @brief Local APIC NMI Struktur in der MADT (Typ 4). */
typedef struct {
    madt_entry_header_t header; /**< MADT-Eintragsheader. */
    uint8_t processor_id;       /**< ACPI-Prozessor-ID (0xFF für alle). */
    uint16_t flags;             /**< Polarität und Trigger-Modus. */
    uint8_t lintin;             /**< LINTIN-Pin am Local APIC (0 oder 1). */
} __attribute__((packed)) madt_lapic_nmi_entry_t;

/** @brief Local APIC Address Override Struktur in der MADT (Typ 5). */
typedef struct {
    madt_entry_header_t header;  /**< MADT-Eintragsheader. */
    uint16_t reserved;           /**< Reserviert. */
    uint64_t local_apic_address; /**< 64-Bit-Basisadresse des Local APIC. */
} __attribute__((packed)) madt_lapic_address_override_entry_t;

/** @brief Local x2APIC Struktur in der MADT (Typ 9). */
typedef struct {
    madt_entry_header_t header; /**< MADT-Eintragsheader. */
    uint16_t reserved;          /**< Reserviert. */
    uint32_t x2apic_id;         /**< x2APIC ID. */
    uint32_t flags;             /**< Flags (z. B. Enabled). */
    uint32_t acpi_id;           /**< ACPI-Prozessor-ID. */
} __attribute__((packed)) madt_lx2apic_entry_t;

/**
 * @struct madt_parsed_t
 * @brief Hilfsstruktur mit Zeigern und Zählern aller geparsten MADT-Einträge.
 */
typedef struct {
    madt_lapic_entry_t *lapics;                           /**< Zeiger auf ersten LAPIC-Eintrag. */
    size_t lapic_count;                                   /**< Anzahl der LAPICs. */
    madt_ioapic_entry_t *ioapics;                         /**< Zeiger auf ersten IOAPIC-Eintrag. */
    size_t ioapic_count;                                  /**< Anzahl der IOAPICs. */
    madt_iso_entry_t *isos;                               /**< Zeiger auf ersten ISO-Eintrag. */
    size_t iso_count;                                     /**< Anzahl der ISOs. */
    madt_ioapic_nmi_entry_t *ioapic_nmis;                 /**< Zeiger auf ersten IOAPIC-NMI-Eintrag. */
    size_t ioapic_nmi_count;                              /**< Anzahl der IOAPIC-NMIs. */
    madt_lapic_nmi_entry_t *lapic_nmis;                   /**< Zeiger auf ersten LAPIC-NMI-Eintrag. */
    size_t lapic_nmi_count;                               /**< Anzahl der LAPIC-NMIs. */
    madt_lapic_address_override_entry_t *lapic_overrides; /**< Zeiger auf ersten LAPIC-Override-Eintrag. */
    size_t lapic_override_count;                          /**< Anzahl der LAPIC-Overrides. */
    madt_lx2apic_entry_t *lx2apics;                       /**< Zeiger auf ersten x2APIC-Eintrag. */
    size_t lx2apic_count;                                 /**< Anzahl der x2APICs. */
} madt_parsed_t;

/**
 * @struct hpet_t
 * @brief High Precision Event Timer (HPET) Tabelle.
 */
typedef struct {
    acpi_sdt_header_t header;      /**< Gemeinsamer SDT-Header. */
    uint32_t event_timer_block_id; /**< Timer-Block-ID und -Eigenschaften. */
    acpi_gas_t base_address;       /**< MMIO-Basisadresse des HPET. */
    uint8_t hpet_number;           /**< HPET-Instanznummer. */
    uint16_t main_counter_minimum; /**< Minimaler Zählwert des Hauptzählers. */
    uint8_t page_protection_oem;   /**< Page Protection Flags. */
} __attribute__((packed)) hpet_t;

/**
 * @struct mcfg_entry_t
 * @brief MCFG Tabelle Eintrag.
 */
typedef struct {
    uint64_t base_address;
    uint16_t pci_segment_group;
    uint8_t start_bus_number;
    uint8_t end_bus_number;
    uint32_t reserved;
} __attribute__((packed)) mcfg_entry_t;

/**
 * @struct mcfg_t
 * @brief MCFG Tabelle.
 */
typedef struct {
    acpi_sdt_header_t header;
    uint64_t reserved;
    mcfg_entry_t entries[];
} __attribute__((packed)) mcfg_t;

/**
 * @struct mcfg_entry_runtime_t
 * @brief Runtime-Struktur für MCFG-Einträge inklusive gemappter virtueller Adresse.
 */
typedef struct {
    uint64_t phys_base;
    virt_addr_t virt_base;
    uint16_t pci_segment;
    uint8_t start_bus;
    uint8_t end_bus;
} mcfg_entry_runtime_t;

typedef struct {
    mcfg_entry_runtime_t *entries;
    size_t count;
} mcfg_info_t;

extern rsdp_t *rsdp;
extern rsdt_t *rsdt;
extern xsdt_t *xsdt;
extern fadt_t *fadt;
extern acpi_sdt_header_t *dsdt;
extern madt_t *madt;
extern madt_parsed_t madt_parsed;
extern hpet_t *hpet;
extern mcfg_t *mcfg;
extern mcfg_info_t mcfg_info;

/**
 * @brief Initialisiert das ACPI-Subsystem.
 *
 * Prüft den RSDP, wählt XSDT (64-Bit) oder RSDT (32-Bit) und registriert alle Untertabellen (FADT, MADT, HPET).
 *
 * @param rsdp_phys Physische Adresse der RSDP-Struktur (vom Bootloader ermittelt).
 */
void acpi_init(phys_addr_t rsdp_phys);

/**
 * @brief Führt den System-Shutdown über den ACPI S5-State durch.
 *
 * Parst das `_S5_`-Paket in der DSDT und beschreibt die `pm1a_cnt_blk`/`pm1b_cnt_blk` I/O-Ports mit den SLP_TYP-Werten.
 */
void acpi_power_off();