/**
 * @file acpi.h
 * @brief 64-bit ACPI (Advanced Configuration and Power Interface) structures
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <arch/x86_64/mm/memdef.h>

#define RSDP_SIGNATURE "RSD PTR "
#define FADT_SIGNATURE "FACP"
#define MADT_SIGNATURE "APIC"
#define HPET_SIGNATURE "HPET"

#define MADT_LAPIC_TYPE 0
#define MADT_IOAPIC_TYPE 1
#define MADT_ISO_TYPE 2
#define MADT_IOAPIC_NMI_TYPE 3
#define MADT_LAPIC_NMI_TYPE 4
#define MADT_LAPIC_ADDRESS_OVERRIDE_TYPE 5
#define MADT_LX2APIC_TYPE 9

/**
 * Root System Description Pointer (RSDP) structure
 */
typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) rsdp_t;

/**
 * Common header for ACPI System Description Tables (SDT)
 */
typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

/**
 * Root System Description Table (RSDT) structure (32-bit pointers)
 */
typedef struct {
    acpi_sdt_header_t header;
    uint32_t pointer_to_other_sdt[];
} __attribute__((packed)) rsdt_t;

/**
 * Extended System Description Table (XSDT) structure (64-bit pointers)
 */
typedef struct {
    acpi_sdt_header_t header;
    uint64_t pointer_to_other_sdt[];
} __attribute__((packed)) xsdt_t;

/**
 * Generic Address Structure (GAS)
 */
typedef struct {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t access_size;
    uint64_t address;
} __attribute__((packed)) acpi_gas_t;

/**
 * Fixed ACPI Description Table (FADT) structure
 */
typedef struct {
    acpi_sdt_header_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_tmr_len;
    uint8_t gpe0_blk_len;
    uint8_t gpe1_blk_len;
    uint8_t gpe1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    uint16_t iapc_boot_arch;
    uint8_t reserved2;
    uint32_t flags;
    acpi_gas_t reset_reg;
    uint8_t reset_value;
    uint8_t reserved3[3];
    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;
    acpi_gas_t x_pm1a_evt_blk;
    acpi_gas_t x_pm1b_evt_blk;
    acpi_gas_t x_pm1a_cnt_blk;
    acpi_gas_t x_pm1b_cnt_blk;
    acpi_gas_t x_pm2_cnt_blk;
    acpi_gas_t x_pm_tmr_blk;
    acpi_gas_t x_gpe0_blk;
    acpi_gas_t x_gpe1_blk;
} __attribute__((packed)) fadt_t;

/**
 * Multiple APIC Description Table (MADT) structure
 */
typedef struct {
    acpi_sdt_header_t header;
    uint32_t local_apic_address;
    uint32_t flags;
    uint8_t entries[];
} __attribute__((packed)) madt_t;

typedef struct {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) madt_entry_header_t;

typedef struct {
    madt_entry_header_t header;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) madt_lapic_entry_t;

typedef struct {
    madt_entry_header_t header;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed)) madt_ioapic_entry_t;

typedef struct {
    madt_entry_header_t header;
    uint8_t bus;
    uint8_t source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed)) madt_iso_entry_t;

typedef struct {
    madt_entry_header_t header;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t global_system_interrupt;
} __attribute__((packed)) madt_ioapic_nmi_entry_t;

typedef struct {
    madt_entry_header_t header;
    uint8_t processor_id;
    uint16_t flags;
    uint8_t lintin;
} __attribute__((packed)) madt_lapic_nmi_entry_t;

typedef struct {
    madt_entry_header_t header;
    uint16_t reserved;
    uint64_t local_apic_address;
} __attribute__((packed)) madt_lapic_address_override_entry_t;

typedef struct {
    madt_entry_header_t header;
    uint16_t reserved;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__((packed)) madt_lx2apic_entry_t;

/**
 * Helper structure containing pointers to parsed MADT entries
 */
typedef struct {
    madt_lapic_entry_t* lapics;
    size_t lapic_count;
    madt_ioapic_entry_t* ioapics;
    size_t ioapic_count;
    madt_iso_entry_t* isos;
    size_t iso_count;
    madt_ioapic_nmi_entry_t* ioapic_nmis;
    size_t ioapic_nmi_count;
    madt_lapic_nmi_entry_t* lapic_nmis;
    size_t lapic_nmi_count;
    madt_lapic_address_override_entry_t* lapic_overrides;
    size_t lapic_override_count;
    madt_lx2apic_entry_t* lx2apics;
    size_t lx2apic_count;
} madt_parsed_t;

/**
 * High Precision Event Timer (HPET) structure
 */
typedef struct {
    acpi_sdt_header_t header;
    uint32_t event_timer_block_id;
    acpi_gas_t base_address;
    uint8_t hpet_number;
    uint16_t main_counter_minimum;
    uint8_t page_protection_oem;
} __attribute__((packed)) hpet_t;

extern rsdp_t* rsdp;
extern rsdt_t* rsdt;
extern xsdt_t* xsdt;
extern fadt_t* fadt;
extern acpi_sdt_header_t* dsdt;
extern madt_t* madt;
extern madt_parsed_t madt_parsed;
extern hpet_t* hpet;

void acpi_init(phys_addr_t rsdp_phys);
void acpi_power_off();