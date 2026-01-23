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
 * Global RSDT structure variable
 */
rsdt_t* rsdt;

/*
 * Global FADT structure variable
 */
fadt_t* fadt;

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
static void acpi_find_rsdp(void) {
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
 * A function to verify the RSDT checksum
 * @param rsdt Pointer to the RSDT structure
 * @return True if the checksum is valid, false otherwise
 */
static bool acpi_verify_rsdt_checksum(rsdt_t* rsdt) {
    uint8_t sum = 0;
    for (size_t i = 0; i < rsdt->header.length; i++) {
        sum += ((uint8_t*)rsdt)[i];
    }
    return (sum == 0);
}

/*
 * A function to find and initialize the RSDT structure
 * @param void
 */
static void acpi_find_rsdt(void) {
    if (rsdp == NULL) {
        rsdt = NULL;
        return;
    }

    rsdt = (rsdt_t*)(rsdp->rsdt_address);
    if (!acpi_verify_rsdt_checksum(rsdt)) {
        rsdt = NULL;
    }
}

/*
 * A function to verify a table checksum
 * @param header Pointer to the ACPI SDT Header
 * @return True if the checksum is valid, false otherwise
 */
static bool acpi_verify_sdt_checksum(acpi_sdt_header_t* header) {
    uint8_t sum = 0;
    for (size_t i = 0; i < header->length; i++) {
        sum += ((uint8_t*)header)[i];
    }
    return (sum == 0);
}

/*
 * A function to find a table based on its signature
 * @param signature The signature of the table to find
 * @return Pointer to the ACPI SDT Header if found, NULL otherwise
 */
static acpi_sdt_header_t* acpi_find_table(const char* signature) {
    if (rsdt == NULL) {
        return NULL;
    }

    uint32_t num_sdts = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(uint32_t);
    for (uint32_t i = 0; i < num_sdts; i++) {
        acpi_sdt_header_t* header = (acpi_sdt_header_t*)(rsdt->pointer_to_other_sdt[i]);
        if (memcmp(header->signature, signature, 4) == 0) {
            if (acpi_verify_sdt_checksum(header)) {
                return header;
            } else {
                continue;
            }
        }
    }
    return NULL;
}


/*
 * A function to get the FADT structure
 * @param void
 */
static void acpi_find_fadt(void) {
    acpi_sdt_header_t* header = acpi_find_table(FADT_SIGNATURE);
    if (header == NULL) {
        fadt = NULL;
        return;
    }
    fadt = (fadt_t*)header;
}

/*
 * A function to initialize ACPI
 * @param void
 */
void acpi_init(void) {
    acpi_find_rsdp();
    acpi_find_rsdt();
    acpi_find_fadt();
}

/*
 * A function to get the RSDP structure
 * @param void
 * @return Pointer to the RSDP structure
 */
rsdp_t* acpi_get_rsdp(void) {
    return rsdp;
}

/*
 * A function to get the RSDT structure
 * @param void
 * @return Pointer to the RSDT structure
 */
rsdt_t* acpi_get_rsdt(void) {
    return rsdt;
}

/*
 * A function to get the FADT structure
 * @param void
 * @return Pointer to the FADT structure
 */
fadt_t* acpi_get_fadt(void) {
    return fadt;
}

/*
 * A function to get the century byte from the FADT
 * @param void
 * @return uint8_t: The century byte
 */
uint8_t acpi_get_fadt_century(void) {
    if (fadt == NULL) return 20;
    return fadt->century;
}
