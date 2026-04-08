/**
 * @file acpi.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define RSDP_SIGNATURE "RSD PTR "
#define FADT_SIGNATURE "FACP"
#define MADT_SIGNATURE "APIC"

#define MADT_LAPIC_TYPE 0
#define MADT_IOAPIC_TYPE 1
#define MADT_ISO_TYPE 2
#define MADT_IOAPIC_NMI_TYPE 3
#define MADT_LAPIC_NMI_TYPE 4
#define MADT_LAPIC_ADDRESS_OVERRIDE_TYPE 5
#define MADT_LX2APIC_TYPE 9

/**
 * @brief Root System Description Pointer (RSDP) structure.
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
 * @brief Common header for ACPI System Description Tables (SDT).
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
 * @brief Generic Address Structure (GAS).
 */
typedef struct {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t access_size;
    uint64_t address;
} __attribute__((packed)) gas_t;

/**
 * @brief Root System Description Table (RSDT).
 */
typedef struct {
    acpi_sdt_header_t header;
    uint32_t pointer_to_other_sdt[];
} __attribute__((packed)) rsdt_t;

/**
 * @brief Extended System Description Table (XSDT).
 */
typedef struct {
    acpi_sdt_header_t header;
    uint64_t pointer_to_other_sdt[];
} __attribute__((packed)) xsdt_t;

/**
 * @brief Fixed ACPI Description Table (FADT).
 */
typedef struct {
    acpi_sdt_header_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t  reserved;
    uint8_t  preferred_power_management_profile;
    uint16_t SCI_interrupt;
    uint32_t SMI_command_port;
    uint8_t  acpi_enable;
    uint8_t  acpi_disable;
    uint8_t  s4bios_req;
    uint8_t  pstate_control;
    uint32_t PM1a_event_block;
    uint32_t PM1b_event_block;
    uint32_t PM1a_control_block;
    uint32_t PM1b_control_block;
    uint32_t PM2_control_block;
    uint32_t PM_timer_block;
    uint32_t GPE0_block;
    uint32_t GPE1_block;
    uint8_t  PM1_event_length;
    uint8_t  PM1_control_length;
    uint8_t  PM2_control_length;
    uint8_t  GPE0_length;
    uint8_t  GPE1_length;
    uint8_t  GPE1_base;
    uint8_t  c_state_register;
    uint16_t worst_case_c2_latency;
    uint16_t worst_case_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t  duty_offset;
    uint8_t  duty_width;
    uint8_t  day_alarm;
    uint8_t  month_alarm;
    uint8_t  century;
    uint16_t boot_architecture_flags;
    uint8_t  reserved2;
    uint32_t flags;
    gas_t reset_register;
    uint8_t reset_command;
    uint8_t reserved3[3];
    uint64_t X_firmware_control;
    uint64_t X_dsdt;
    gas_t X_PM1a_event_block;
    gas_t X_PM1b_event_block;
    gas_t X_PM1a_control_block;
    gas_t X_PM1b_control_block;
    gas_t X_PM2_control_block;
    gas_t X_PM_timer_block;
    gas_t X_GPE0_block;
    gas_t X_GPE1_block;
} __attribute__((packed)) fadt_t;

/**
 * @brief Multiple APIC Description Table (MADT).
 */
typedef struct {
    acpi_sdt_header_t header;
    uint32_t local_apic_address;
    uint32_t flags;
} __attribute__((packed)) madt_t;

/**
 * @brief Common header for MADT entries.
 */
typedef struct {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) madt_entry_header_t;

/**
 * @brief Processor Local APIC entry (Type 0).
 */
typedef struct {
    madt_entry_header_t header;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) madt_lapic_entry_t;

/**
 * @brief I/O APIC entry (Type 1).
 */
typedef struct {
    madt_entry_header_t header;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed)) madt_ioapic_entry_t;

/**
 * @brief Interrupt Source Override entry (Type 2).
 */
typedef struct {
    madt_entry_header_t header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed)) madt_iso_entry_t;

/**
 * @brief I/O APIC Non-maskable Interrupt Source entry (Type 3).
 */
typedef struct {
    madt_entry_header_t header;
    uint8_t nmi_source;
    uint8_t reserved;
    uint16_t flags;
    uint32_t global_system_interrupt;
} __attribute__((packed)) madt_ioapic_nmi_entry_t;

/**
 * @brief Local APIC Non-maskable Interrupts entry (Type 4).
 */
typedef struct {
    madt_entry_header_t header;
    uint8_t processor_id;
    uint16_t flags;
    uint8_t lintin;
} __attribute__((packed)) madt_lapic_nmi_entry_t;

/**
 * @brief Local APIC Address Override entry (Type 5).
 */
typedef struct {
    madt_entry_header_t header;
    uint16_t reserved;
    uint64_t local_apic_address;
} __attribute__((packed)) madt_lapic_address_override_entry_t;

/**
 * @brief Processor Local x2APIC entry (Type 9).
 */
typedef struct {
    madt_entry_header_t header;
    uint16_t reserved;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__((packed)) madt_lx2apic_entry_t;

/**
 * @brief Helper structure containing pointers to parsed MADT entries.
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
    madt_lapic_address_override_entry_t* lapic_address_overrides;
    size_t lapic_address_override_count;
    madt_lx2apic_entry_t* lx2apics;
    size_t lx2apic_count;
} __attribute__((packed)) madt_parsed_t;

extern rsdp_t* rsdp;
extern rsdt_t* rsdt;
extern xsdt_t* xsdt;
extern fadt_t* fadt;
extern madt_t* madt;
extern madt_parsed_t madt_parsed;

void acpi_init(rsdp_t * rsdp_ptr);
void acpi_dump_info();
void acpi_dump_fadt();
void acpi_dump_madt();