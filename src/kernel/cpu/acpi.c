/*
 * @file acpi.c
 * @brief ACPI (Advanced Configuration and Power Interface) implementation
 * @author friedrichOsDev
 */

#include <acpi.h>
#include <string.h>
#include <serial.h>
#include <heap.h>

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
 * Global MADT structure variable
 */
madt_t* madt;

/*
 * Global MADT parsed structure variable
 */
madt_parsed_t madt_parsed;

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
 * A function to find and initialize the MADT structure
 * @param void
 */
static void acpi_find_madt(void) {
    acpi_sdt_header_t* header = acpi_find_table(MADT_SIGNATURE);
    if (header == NULL) {
        madt = NULL;
        return;
    }
    madt = (madt_t*)header;
}

/*
 * A function to parse the MADT
 * @param void
 */
static void acpi_parse_madt(void) {
    madt_parsed.lapics = NULL;
    madt_parsed.lapic_count = 0;
    madt_parsed.ioapics = NULL;
    madt_parsed.ioapic_count = 0;
    madt_parsed.isos = NULL;
    madt_parsed.iso_count = 0;
    madt_parsed.ioapic_nmis = NULL;
    madt_parsed.ioapic_nmi_count = 0;
    madt_parsed.lapic_nmis = NULL;
    madt_parsed.lapic_nmi_count = 0;
    madt_parsed.lapic_address_overrides = NULL;
    madt_parsed.lapic_address_override_count = 0;
    madt_parsed.lx2apics = NULL;
    madt_parsed.lx2apic_count = 0;

    if (madt == NULL) return;

    uint8_t* ptr = (uint8_t*)madt + sizeof(madt_t);
    uint8_t* end = (uint8_t*)madt + madt->header.length;

    size_t lapic_count = 0;
    size_t ioapic_count = 0;
    size_t iso_count = 0;
    size_t ioapic_nmi_count = 0;
    size_t lapic_nmi_count = 0;
    size_t lapic_address_override_count = 0;
    size_t lx2apic_count = 0;

    while (ptr < end) {
        madt_entry_header_t* header = (madt_entry_header_t*)ptr;
        switch (header->type) {
            case MADT_LAPIC_TYPE: {
                lapic_count++;
                break;
            }
            case MADT_IOAPIC_TYPE: {
                ioapic_count++;
                break;
            }
            case MADT_ISO_TYPE: {
                iso_count++;
                break;
            }
            case MADT_IOAPIC_NMI_TYPE: {
                ioapic_nmi_count++;
                break;
            }
            case MADT_LAPIC_NMI_TYPE: {
                lapic_nmi_count++;
                break;
            }
            case MADT_LAPIC_ADDRESS_OVERRIDE_TYPE: {
                lapic_address_override_count++;
                break;
            }
            case MADT_LX2APIC_TYPE: {
                lx2apic_count++;
                break;
            }
            default: {
                break;
            }
        }
        ptr += header->length;
    }

    madt_parsed.lapic_count = 0;
    madt_parsed.ioapic_count = 0;
    madt_parsed.iso_count = 0;
    madt_parsed.ioapic_nmi_count = 0;
    madt_parsed.lapic_nmi_count = 0;
    madt_parsed.lapic_address_override_count = 0;
    madt_parsed.lx2apic_count = 0;
    
    ptr = (uint8_t*)madt + sizeof(madt_t);
    end = (uint8_t*)madt + madt->header.length;

    madt_lapic_entry_t* lapics = kzalloc(sizeof(madt_lapic_entry_t) * lapic_count);
    madt_ioapic_entry_t* ioapics = kzalloc(sizeof(madt_ioapic_entry_t) * ioapic_count);
    madt_iso_entry_t* isos = kzalloc(sizeof(madt_iso_entry_t) * iso_count);
    madt_ioapic_nmi_entry_t* ioapic_nmis = kzalloc(sizeof(madt_ioapic_nmi_entry_t) * ioapic_nmi_count);
    madt_lapic_nmi_entry_t* lapic_nmis = kzalloc(sizeof(madt_lapic_nmi_entry_t) * lapic_nmi_count);
    madt_lapic_address_override_entry_t* lapic_address_overrides = kzalloc(sizeof(madt_lapic_address_override_entry_t) * lapic_address_override_count);
    madt_lx2apic_entry_t* lx2apics = kzalloc(sizeof(madt_lx2apic_entry_t) * lx2apic_count);

    while (ptr < end) {
        madt_entry_header_t* header = (madt_entry_header_t*)ptr;
        switch (header->type) {
            case MADT_LAPIC_TYPE: {
                madt_lapic_entry_t* lapic = (madt_lapic_entry_t*)ptr;
                memcpy(&lapics[madt_parsed.lapic_count++], lapic, sizeof(madt_lapic_entry_t));
                break;
            }
            case MADT_IOAPIC_TYPE: {
                madt_ioapic_entry_t* ioapic = (madt_ioapic_entry_t*)ptr;
                memcpy(&ioapics[madt_parsed.ioapic_count++], ioapic, sizeof(madt_ioapic_entry_t));
                break;
            }
            case MADT_ISO_TYPE: {
                madt_iso_entry_t* iso = (madt_iso_entry_t*)ptr;
                memcpy(&isos[madt_parsed.iso_count++], iso, sizeof(madt_iso_entry_t));
                break;
            }
            case MADT_IOAPIC_NMI_TYPE: {
                madt_ioapic_nmi_entry_t* ioapic_nmi = (madt_ioapic_nmi_entry_t*)ptr;
                memcpy(&ioapic_nmis[madt_parsed.ioapic_nmi_count++], ioapic_nmi, sizeof(madt_ioapic_nmi_entry_t));
                break;
            }
            case MADT_LAPIC_NMI_TYPE: {
                madt_lapic_nmi_entry_t* lapic_nmi = (madt_lapic_nmi_entry_t*)ptr;
                memcpy(&lapic_nmis[madt_parsed.lapic_nmi_count++], lapic_nmi, sizeof(madt_lapic_nmi_entry_t));
                break;
            }
            case MADT_LAPIC_ADDRESS_OVERRIDE_TYPE: {
                madt_lapic_address_override_entry_t* lapic_address_override = (madt_lapic_address_override_entry_t*)ptr;
                memcpy(&lapic_address_overrides[madt_parsed.lapic_address_override_count++], lapic_address_override, sizeof(madt_lapic_address_override_entry_t));
                break;
            }
            case MADT_LX2APIC_TYPE: {
                madt_lx2apic_entry_t* lx2apic = (madt_lx2apic_entry_t*)ptr;
                memcpy(&lx2apics[madt_parsed.lx2apic_count++], lx2apic, sizeof(madt_lx2apic_entry_t));
                break;
            }
            default: {
                break;
            }
        }
        ptr += header->length;
    }

    madt_parsed.lapics = lapics;
    madt_parsed.lapic_count = lapic_count;
    madt_parsed.ioapics = ioapics;
    madt_parsed.ioapic_count = ioapic_count;
    madt_parsed.isos = isos;
    madt_parsed.iso_count = iso_count;
    madt_parsed.ioapic_nmis = ioapic_nmis;
    madt_parsed.ioapic_nmi_count = ioapic_nmi_count;
    madt_parsed.lapic_nmis = lapic_nmis;
    madt_parsed.lapic_nmi_count = lapic_nmi_count;
    madt_parsed.lapic_address_overrides = lapic_address_overrides;
    madt_parsed.lapic_address_override_count = lapic_address_override_count;
    madt_parsed.lx2apics = lx2apics;
    madt_parsed.lx2apic_count = lx2apic_count;
}

/*
 * A function to initialize ACPI
 * @param void
 */
void acpi_init(void) {
    acpi_find_rsdp();
    acpi_find_rsdt();
    acpi_find_fadt();
    acpi_find_madt();
    acpi_parse_madt();
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
 * A function to get the MADT structure
 * @param void
 * @return Pointer to the MADT structure
 */
madt_t* acpi_get_madt(void) {
    return madt;
}

/*
 * A function to get the parsed MADT structure
 * @param void
 * @return Pointer to the parsed MADT structure
 */
madt_parsed_t* acpi_get_madt_parsed(void) {
    return &madt_parsed;
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
