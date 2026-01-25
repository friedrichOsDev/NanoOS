/*
 * @file acpi.h
 * @brief Header file for ACPI (Advanced Configuration and Power Interface) implementation
 * @author friedrichOsDev
 */

#ifndef ACPI_H
#define ACPI_H

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

/*
 * Structure representing the RSDP (Root System Description Pointer)
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) rsdp_t;

/*
 * Structure representing the ACPI SDT Header
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

/*
 * Structure representing the GAS (Generic Address Structure)
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t access_size;
    uint64_t address;
} __attribute__((packed)) gas_t;

/*
 * Structure representing the RSDT (Root System Description Table)
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    acpi_sdt_header_t header;
    uint32_t pointer_to_other_sdt[];
} __attribute__((packed)) rsdt_t;

/*
 * Structure representing the XSDT (Extended System Description Table)
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    acpi_sdt_header_t header;
    uint64_t pointer_to_other_sdt[];
} __attribute__((packed)) xsdt_t;

/*
 * Structure representing the FADT (Fixed ACPI Description Table)
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
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

/*
 * Structure representing the MADT (Multiple APIC Description Table)
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    acpi_sdt_header_t header;
    uint32_t local_apic_address;
    uint32_t flags;
} __attribute__((packed)) madt_t;

/*
 * Structure representing the MADT entry header
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) madt_entry_header_t;

/*
 * Structure representing a Processor Local APIC entry in MADT
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    madt_entry_header_t header;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) madt_lapic_entry_t;

/*
 * Structure representing an I/O APIC entry in MADT
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    madt_entry_header_t header;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed)) madt_ioapic_entry_t;

/*
 * Structure representing an Interrupt Source Override (ISO) entry in MADT
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    madt_entry_header_t header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed)) madt_iso_entry_t;

/*
 * Structure representing a I/O APIC Non-maskable Interrupt (NMI) entry in MADT
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    madt_entry_header_t header;
    uint8_t nmi_source;
    uint8_t reserved;
    uint16_t flags;
    uint32_t global_system_interrupt;
} __attribute__((packed)) madt_ioapic_nmi_entry_t;

/*
 * Structure representing a Local APIC NMI entry in MADT
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    madt_entry_header_t header;
    uint8_t processor_id;
    uint16_t flags;
    uint8_t lintin;
} __attribute__((packed)) madt_lapic_nmi_entry_t;

/*
 * Structure representing a Local APIC Address Override entry in MADT
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    madt_entry_header_t header;
    uint16_t reserved;
    uint64_t local_apic_address;
} __attribute__((packed)) madt_lapic_address_override_entry_t;

/*
 * Structure representing a Processor Local x2APIC entry in MADT
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    madt_entry_header_t header;
    uint16_t reserved;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__((packed)) madt_lx2apic_entry_t;

/*
 * Structure to save the parsed MADT
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
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

void acpi_init(void);
rsdp_t* acpi_get_rsdp(void);
rsdt_t* acpi_get_rsdt(void);
fadt_t* acpi_get_fadt(void);
madt_t* acpi_get_madt(void);
madt_parsed_t* acpi_get_madt_parsed(void);
uint8_t acpi_get_fadt_century(void);

#endif // ACPI_H