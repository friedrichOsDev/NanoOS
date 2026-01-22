/*
 * @file acpi.c
 * @brief ACPI (Advanced Configuration and Power Interface) implementation
 * @author friedrichOsDev
 */

#include <acpi.h>
#include <string.h>
#include <serial.h>

/*
 * Global RSDP structure variable
 */
rsdp_t* rsdp;

/*
 * A function to initialize ACPI
 * @param void
 */
void acpi_init(void) {
    acpi_find_rsdp();
}

/*
 * A function to verify the RSDP checksum
 * @param rsdp Pointer to the RSDP structure
 * @return True if the checksum is valid, false otherwise
 */
static bool acpi_verify_rsdp_checksum(rsdp_t* rsdp) {
    uint8_t sum = 0;
    for (size_t i = 0; i < 20; i++) { // RSDP version 1 has a length of 20 bytes
        sum += ((uint8_t*)rsdp)[i];
    }
    
    if (sum == 0) {
        if (rsdp->revision >= 2) {
            sum = 0;
            for (size_t i = 0; i < rsdp->length; i++) {
                sum += ((uint8_t*)rsdp)[i];
            }
            return (sum == 0);
        } else {
            return true;
        }
    }
    return false;
}

/*
 * A function to find and initialize the RSDP structure
 * @param void
 */
void acpi_find_rsdp(void) {
    // Search for RSDP in the BIOS memory area (0x000E0000 to 0x000FFFFF)
    for (uint32_t addr = 0x000E0000; addr < 0x00100000; addr += 16) {
        rsdp_t* current_rsdp = (rsdp_t*)addr;
        if (memcmp(current_rsdp->signature, RSDP_SIGNATURE, 8) == 0) {
            if (acpi_verify_rsdp_checksum(current_rsdp)) {
                rsdp = current_rsdp;
                return;
            } else {
                continue;
            }
        }
    }

    // If not found, search in the Extended BIOS Data Area (EBDA)
    uint16_t* ebda_addr_ptr = (uint16_t*)0x40E;
    uint32_t ebda_addr = (*ebda_addr_ptr) << 4;
    for (uint32_t addr = ebda_addr; addr < ebda_addr + 1024; addr += 16) {
        rsdp_t* current_rsdp = (rsdp_t*)addr;
        if (memcmp(current_rsdp->signature, RSDP_SIGNATURE, 8) == 0) {
            if (acpi_verify_rsdp_checksum(current_rsdp)) {
                rsdp = current_rsdp;
                return;
            } else {
                continue;
            }
        }
    }

    rsdp = NULL; // RSDP not found
}

/*
 * A function to get the RSDP structure
 * @param void
 * @return Pointer to the RSDP structure
 */
rsdp_t* acpi_get_rsdp(void) {
    return rsdp;
}