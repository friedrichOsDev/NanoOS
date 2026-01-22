/*
 * @file acpi.h
 * @brief Header file for ACPI (Advanced Configuration and Power Interface) implementation
 * @author friedrichOsDev
 */

#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
#include <stdbool.h>

#define RSDP_SIGNATURE "RSD PTR "

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
 * Structure representing the RSDT (Root System Description Table)
 * @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
 */
typedef struct {
    acpi_sdt_header_t header;
    uint32_t pointer_to_other_sdt[];
} __attribute__((packed)) rsdt_t;

void acpi_init(void);
rsdp_t* acpi_get_rsdp(void);
rsdt_t* acpi_get_rsdt(void);

#endif // ACPI_H