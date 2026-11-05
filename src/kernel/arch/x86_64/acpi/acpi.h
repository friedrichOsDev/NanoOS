/**
 * @file acpi.h
 * @brief 64-bit ACPI structures and definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

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

/** @brief Root System Description Pointer (RSDP). */
typedef struct {
    char signature[8];         /**< Signature "RSD PTR ". */
    uint8_t checksum;          /**< Checksum of the first 20 bytes. */
    char oem_id[6];            /**< OEM identifier. */
    uint8_t revision;          /**< ACPI revision (0/1 = ACPI 1.0, >=2 = ACPI 2.0+). */
    uint32_t rsdt_address;     /**< Physical address of the 32-bit RSDT. */
    uint32_t length;           /**< Total length of the RSDP structure. */
    uint64_t xsdt_address;     /**< Physical address of the 64-bit XSDT. */
    uint8_t extended_checksum; /**< Extended checksum. */
    uint8_t reserved[3];       /**< Reserved bytes. */
} __attribute__((packed)) rsdp_t;

/** @brief Common header for all ACPI System Description Tables (SDT). */
typedef struct {
    char signature[4];         /**< Table signature (e.g. "APIC", "FACP"). */
    uint32_t length;           /**< Total length of the table in bytes, including the header. */
    uint8_t revision;          /**< Table revision. */
    uint8_t checksum;          /**< Checksum for the entire table. */
    char oem_id[6];            /**< OEM ID. */
    char oem_table_id[8];      /**< OEM table ID. */
    uint32_t oem_revision;     /**< OEM revision. */
    uint32_t creator_id;       /**< Creator ID (e.g. ASL compiler). */
    uint32_t creator_revision; /**< Creator revision. */
} __attribute__((packed)) acpi_sdt_header_t;

/** @brief Root System Description Table (32-bit pointers). */
typedef struct {
    acpi_sdt_header_t header;        /**< Common SDT header. */
    uint32_t pointer_to_other_sdt[]; /**< Array of 32-bit addresses of other ACPI tables. */
} __attribute__((packed)) rsdt_t;

/** @brief Extended System Description Table (64-bit pointers). */
typedef struct {
    acpi_sdt_header_t header;        /**< Common SDT header. */
    uint64_t pointer_to_other_sdt[]; /**< Array of 64-bit addresses of other ACPI tables. */
} __attribute__((packed)) xsdt_t;

/** @brief Generic Address Structure (GAS). */
typedef struct {
    uint8_t address_space_id;    /**< Address space ID (0 = System Memory, 1 = System I/O, etc.). */
    uint8_t register_bit_width;  /**< Register width in bits. */
    uint8_t register_bit_offset; /**< Bit offset within the register. */
    uint8_t access_size;         /**< Access size restriction. */
    uint64_t address;            /**< 64-bit register address. */
} __attribute__((packed)) acpi_gas_t;

/** @brief Fixed ACPI Description Table (FADT). */
typedef struct {
    acpi_sdt_header_t header;     /**< Common SDT header. */
    uint32_t firmware_ctrl;       /**< Physical address of FACS. */
    uint32_t dsdt;                /**< 32-bit address of DSDT. */
    uint8_t reserved;             /**< Reserved. */
    uint8_t preferred_pm_profile; /**< Preferred power management profile. */
    uint16_t sci_int;             /**< SCI interrupt vector. */
    uint32_t smi_cmd;             /**< SMI command port. */
    uint8_t acpi_enable;          /**< Value to enable ACPI. */
    uint8_t acpi_disable;         /**< Value to disable ACPI. */
    uint8_t s4bios_req;           /**< S4BIOS request value. */
    uint8_t pstate_cnt;           /**< P-state control value. */
    uint32_t pm1a_evt_blk;        /**< PM1a event block address. */
    uint32_t pm1b_evt_blk;        /**< PM1b event block address. */
    uint32_t pm1a_cnt_blk;        /**< PM1a control block address. */
    uint32_t pm1b_cnt_blk;        /**< PM1b control block address. */
    uint32_t pm2_cnt_blk;         /**< PM2 control block address. */
    uint32_t pm_tmr_blk;          /**< PM timer block address. */
    uint32_t gpe0_blk;            /**< GPE0 block address. */
    uint32_t gpe1_blk;            /**< GPE1 block address. */
    uint8_t pm1_evt_len;          /**< Length of PM1 event block. */
    uint8_t pm1_cnt_len;          /**< Length of PM1 control block. */
    uint8_t pm2_cnt_len;          /**< Length of PM2 control block. */
    uint8_t pm_tmr_len;           /**< Length of PM timer. */
    uint8_t gpe0_blk_len;         /**< Length of GPE0 block. */
    uint8_t gpe1_blk_len;         /**< Length of GPE1 block. */
    uint8_t gpe1_base;            /**< GPE1 base offset. */
    uint8_t cst_cnt;              /**< C-state control. */
    uint16_t p_lvl2_lat;          /**< C2 latency. */
    uint16_t p_lvl3_lat;          /**< C3 latency. */
    uint16_t flush_size;          /**< Flush size. */
    uint16_t flush_stride;        /**< Flush stride. */
    uint8_t duty_offset;          /**< Duty cycle offset. */
    uint8_t duty_width;           /**< Duty cycle width. */
    uint8_t day_alrm;             /**< RTC day alarm index. */
    uint8_t mon_alrm;             /**< RTC month alarm index. */
    uint8_t century;              /**< RTC century index. */
    uint16_t iapc_boot_arch;      /**< IA-PC boot architecture flags. */
    uint8_t reserved2;            /**< Reserved. */
    uint32_t flags;               /**< Global ACPI feature flags. */
    acpi_gas_t reset_reg;         /**< Reset register GAS. */
    uint8_t reset_value;          /**< Reset command value. */
    uint8_t reserved3[3];         /**< Reserved. */
    uint64_t x_firmware_ctrl;     /**< 64-bit address of FACS. */
    uint64_t x_dsdt;              /**< 64-bit address of DSDT. */
    acpi_gas_t x_pm1a_evt_blk;    /**< 64-bit PM1a event block GAS. */
    acpi_gas_t x_pm1b_evt_blk;    /**< 64-bit PM1b event block GAS. */
    acpi_gas_t x_pm1a_cnt_blk;    /**< 64-bit PM1a control block GAS. */
    acpi_gas_t x_pm1b_cnt_blk;    /**< 64-bit PM1b control block GAS. */
    acpi_gas_t x_pm2_cnt_blk;     /**< 64-bit PM2 control block GAS. */
    acpi_gas_t x_pm_tmr_blk;      /**< 64-bit PM timer block GAS. */
    acpi_gas_t x_gpe0_blk;        /**< 64-bit GPE0 block GAS. */
    acpi_gas_t x_gpe1_blk;        /**< 64-bit GPE1 block GAS. */
} __attribute__((packed)) fadt_t;

/** @brief Multiple APIC Description Table (MADT). */
typedef struct {
    acpi_sdt_header_t header;    /**< Common SDT header. */
    uint32_t local_apic_address; /**< Physical base address of the Local APIC. */
    uint32_t flags;              /**< APIC flags (e.g. Dual PIC PC AT flag). */
    uint8_t entries[];           /**< Variable array of MADT entries. */
} __attribute__((packed)) madt_t;

/** @brief Common header for sub-structures inside the MADT. */
typedef struct {
    uint8_t type;   /**< Type of the MADT entry. */
    uint8_t length; /**< Length of the entry in bytes. */
} __attribute__((packed)) madt_entry_header_t;

/** @brief Local APIC structure in the MADT (Type 0). */
typedef struct {
    madt_entry_header_t header; /**< MADT entry header. */
    uint8_t acpi_processor_id;  /**< ACPI processor ID. */
    uint8_t apic_id;            /**< Local APIC ID. */
    uint32_t flags;             /**< Status flags (e.g. Enabled). */
} __attribute__((packed)) madt_lapic_entry_t;

/** @brief I/O APIC structure in the MADT (Type 1). */
typedef struct {
    madt_entry_header_t header;            /**< MADT entry header. */
    uint8_t ioapic_id;                     /**< I/O APIC ID. */
    uint8_t reserved;                      /**< Reserved. */
    uint32_t ioapic_address;               /**< Physical MMIO address of the I/O APIC. */
    uint32_t global_system_interrupt_base; /**< Global System Interrupt (GSI) base. */
} __attribute__((packed)) madt_ioapic_entry_t;

/** @brief Interrupt Source Override structure in the MADT (Type 2). */
typedef struct {
    madt_entry_header_t header;       /**< MADT entry header. */
    uint8_t bus;                      /**< Bus source (always 0 = ISA). */
    uint8_t source;                   /**< ISA IRQ source. */
    uint32_t global_system_interrupt; /**< Target GSI line. */
    uint16_t flags;                   /**< Polarity and trigger mode flags. */
} __attribute__((packed)) madt_iso_entry_t;

/** @brief I/O APIC NMI Source structure in the MADT (Type 3). */
typedef struct {
    madt_entry_header_t header;       /**< MADT entry header. */
    uint8_t ioapic_id;                /**< I/O APIC ID. */
    uint8_t reserved;                 /**< Reserved. */
    uint32_t global_system_interrupt; /**< GSI number for NMI. */
} __attribute__((packed)) madt_ioapic_nmi_entry_t;

/** @brief Local APIC NMI structure in the MADT (Type 4). */
typedef struct {
    madt_entry_header_t header; /**< MADT entry header. */
    uint8_t processor_id;       /**< ACPI processor ID (0xFF for all). */
    uint16_t flags;             /**< Polarity and trigger mode flags. */
    uint8_t lintin;             /**< Local APIC LINTIN pin (0 or 1). */
} __attribute__((packed)) madt_lapic_nmi_entry_t;

/** @brief Local APIC Address Override structure in the MADT (Type 5). */
typedef struct {
    madt_entry_header_t header;  /**< MADT entry header. */
    uint16_t reserved;           /**< Reserved. */
    uint64_t local_apic_address; /**< 64-bit physical address of the Local APIC. */
} __attribute__((packed)) madt_lapic_address_override_entry_t;

/** @brief Local x2APIC structure in the MADT (Type 9). */
typedef struct {
    madt_entry_header_t header; /**< MADT entry header. */
    uint16_t reserved;          /**< Reserved. */
    uint32_t x2apic_id;         /**< x2APIC ID. */
    uint32_t flags;             /**< Flags (e.g. Enabled). */
    uint32_t acpi_id;           /**< ACPI processor ID. */
} __attribute__((packed)) madt_lx2apic_entry_t;

/** @brief Helper structure containing pointers and counts for all parsed MADT entries. */
typedef struct {
    madt_lapic_entry_t *lapics;                           /**< Pointer to the first LAPIC entry. */
    size_t lapic_count;                                   /**< Number of LAPICs. */
    madt_ioapic_entry_t *ioapics;                         /**< Pointer to the first IOAPIC entry. */
    size_t ioapic_count;                                  /**< Number of IOAPICs. */
    madt_iso_entry_t *isos;                               /**< Pointer to the first ISO entry. */
    size_t iso_count;                                     /**< Number of ISOs. */
    madt_ioapic_nmi_entry_t *ioapic_nmis;                 /**< Pointer to the first IOAPIC NMI entry. */
    size_t ioapic_nmi_count;                              /**< Number of IOAPIC NMIs. */
    madt_lapic_nmi_entry_t *lapic_nmis;                   /**< Pointer to the first LAPIC NMI entry. */
    size_t lapic_nmi_count;                               /**< Number of LAPIC NMIs. */
    madt_lapic_address_override_entry_t *lapic_overrides; /**< Pointer to the first LAPIC override entry. */
    size_t lapic_override_count;                          /**< Number of LAPIC overrides. */
    madt_lx2apic_entry_t *lx2apics;                       /**< Pointer to the first x2APIC entry. */
    size_t lx2apic_count;                                 /**< Number of x2APICs. */
} madt_parsed_t;

/** @brief High Precision Event Timer (HPET) table. */
typedef struct {
    acpi_sdt_header_t header;      /**< Common SDT header. */
    uint32_t event_timer_block_id; /**< Timer block ID and configuration properties. */
    acpi_gas_t base_address;       /**< MMIO base address of the HPET. */
    uint8_t hpet_number;           /**< HPET instance number. */
    uint16_t main_counter_minimum; /**< Minimum tick count of the main counter. */
    uint8_t page_protection_oem;   /**< Page protection and OEM attribute flags. */
} __attribute__((packed)) hpet_t;

/** @brief MCFG (Memory-Mapped Configuration) table entry. */
typedef struct {
    uint64_t base_address;       /**< Base address of the PCI Express ECAM space. */
    uint16_t pci_segment_group;  /**< PCI segment group number. */
    uint8_t start_bus_number;    /**< Start bus number. */
    uint8_t end_bus_number;      /**< End bus number. */
    uint32_t reserved;           /**< Reserved. */
} __attribute__((packed)) mcfg_entry_t;

/** @brief Memory-Mapped Configuration Table (MCFG). */
typedef struct {
    acpi_sdt_header_t header;    /**< Common SDT header. */
    uint64_t reserved;           /**< Reserved. */
    mcfg_entry_t entries[];      /**< Variable array of MCFG entries. */
} __attribute__((packed)) mcfg_t;

/** @brief Runtime structure for MCFG entries including mapped virtual addresses. */
typedef struct {
    uint64_t phys_base;          /**< Physical base address. */
    virt_addr_t virt_base;       /**< Mapped virtual base address. */
    uint16_t pci_segment;        /**< PCI segment group. */
    uint8_t start_bus;           /**< Start bus number. */
    uint8_t end_bus;             /**< End bus number. */
} mcfg_entry_runtime_t;

/** @brief Container holding runtime MCFG information. */
typedef struct {
    mcfg_entry_runtime_t *entries; /**< Pointer to runtime MCFG entries array. */
    size_t count;                  /**< Number of MCFG entries. */
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
