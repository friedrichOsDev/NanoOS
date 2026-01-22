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
 * @note @note The __attribute__((packed)) directive is used to prevent the compiler from adding padding bytes
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

void acpi_init(void);
void acpi_find_rsdp(void);
rsdp_t* acpi_get_rsdp(void);

#endif // ACPI_H