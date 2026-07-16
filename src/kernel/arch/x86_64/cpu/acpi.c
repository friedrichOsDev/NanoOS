/**
 * @file acpi.c
 * @brief 64-bit ACPI (Advanced Configuration and Power Interface)
 * @author friedrichOsDev
 */

#include <stdbool.h>
#include <arch/x86_64/cpu/acpi.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/vmm.h>
#include <lib/string.h>
#include <lib/io.h>

rsdp_t* rsdp = NULL;
rsdt_t* rsdt = NULL;
xsdt_t* xsdt = NULL;
fadt_t* fadt = NULL;
acpi_sdt_header_t* dsdt = NULL;
madt_t* madt = NULL;
madt_parsed_t madt_parsed;
hpet_t* hpet = NULL;

/**
 * Verifies the RSDP checksum(s)
 * @param target_rsdp The RSDP structure to verify
 * @return True if valid structure, False if invalid checksum(s)
 */
static bool acpi_verify_rsdp_checksum(rsdp_t* target_rsdp) {
    if (!target_rsdp || memcmp(target_rsdp->signature, RSDP_SIGNATURE, 8) != 0) {
        return false;
    }

    uint8_t* bytes = (uint8_t*)target_rsdp;
    uint8_t sum = 0;
    for (size_t i = 0; i < 20; i++) {
        sum += bytes[i];
    }

    if (sum != 0) {
        serial_printf(COM1, "ACPI: RSDP base checksum invalid\n");
        return false;
    }

    if (target_rsdp->revision >= 2) {
        uint8_t ext_sum = 0;
        for (size_t i = 0; i < target_rsdp->length; i++) {
            ext_sum += bytes[i];
        }
        if (ext_sum != 0) {
            serial_printf(COM1, "ACPI: RSDP extended checksum invalid\n");
            return false;
        }
    }
    return true;
}

/**
 * Verifies the SDT Header checksum
 * @param header The SDT Header to verify
 * @return True if valid header, False if invalid header
 */
static bool acpi_verify_sdt_checksum(acpi_sdt_header_t* header) {
    if (!header) return false;
    uint8_t sum = 0;
    uint8_t* bytes = (uint8_t*)header;
    for (uint32_t i = 0; i < header->length; i++) {
        sum += bytes[i];
    }
    return sum == 0;
}

/**
 * Parses the MADT structure
 * @param target_madt The MADT structure to parse
 */
static void acpi_parse_madt(madt_t* target_madt) {
    if (!target_madt) return;

    memset(&madt_parsed, 0, sizeof(madt_parsed_t));
    uint8_t* ptr = target_madt->entries;
    uint8_t* end = (uint8_t*)target_madt + target_madt->header.length;

    while (ptr < end) {
        madt_entry_header_t* entry = (madt_entry_header_t*)ptr;
        if (entry->length == 0) {
            serial_printf(COM1, "ACPI: MADT entry with length 0\n");
            break;
        }

        switch (entry->type) {
            case MADT_LAPIC_TYPE:
                if (madt_parsed.lapic_count == 0) {
                    madt_parsed.lapics = (madt_lapic_entry_t*)entry;
                }
                madt_parsed.lapic_count++;
                break;
            case MADT_IOAPIC_TYPE:
                if (madt_parsed.ioapic_count == 0) {
                    madt_parsed.ioapics = (madt_ioapic_entry_t*)entry;
                }
                madt_parsed.ioapic_count++;
                break;
            case MADT_ISO_TYPE:
                if (madt_parsed.iso_count == 0) {
                    madt_parsed.isos = (madt_iso_entry_t*)entry;
                }
                madt_parsed.iso_count++;
                break;
            case MADT_IOAPIC_NMI_TYPE:
                if (madt_parsed.ioapic_nmi_count == 0) {
                    madt_parsed.ioapic_nmis = (madt_ioapic_nmi_entry_t*)entry;
                }
                madt_parsed.ioapic_nmi_count++;
                break;
            case MADT_LAPIC_NMI_TYPE:
                if (madt_parsed.lapic_nmi_count == 0) {
                    madt_parsed.lapic_nmis = (madt_lapic_nmi_entry_t*)entry;
                }
                madt_parsed.lapic_nmi_count++;
                break;
            case MADT_LAPIC_ADDRESS_OVERRIDE_TYPE:
                if (madt_parsed.lapic_override_count == 0) {
                    madt_parsed.lapic_overrides = (madt_lapic_address_override_entry_t*)entry;
                }
                madt_parsed.lapic_override_count++;
                break;
            case MADT_LX2APIC_TYPE:
                if (madt_parsed.lx2apic_count == 0) {
                    madt_parsed.lx2apics = (madt_lx2apic_entry_t*)entry;
                }
                madt_parsed.lx2apic_count++;
                break;
            default:
                break;
        }
        ptr += entry->length;
    }

    serial_printf(COM1, "ACPI: loaded MADT (LAPICs: %d, IOAPICs: %d, ISOs: %d)\n", madt_parsed.lapic_count, madt_parsed.ioapic_count, madt_parsed.iso_count);
}

/**
 * Registers a ACPI table
 * @param phys_header The physical address of the Header
 */
static void acpi_register_table(acpi_sdt_header_t* phys_header) {
    acpi_sdt_header_t* header = (acpi_sdt_header_t*)P2V((uint64_t)phys_header);

    if (!acpi_verify_sdt_checksum(header)) {
        serial_printf(COM1, "ACPI: SDT checksum invalid for: %c%c%c%c\n", header->signature[0], header->signature[1], header->signature[2], header->signature[3]);
        return;
    }

    if (memcmp(header->signature, FADT_SIGNATURE, 4) == 0) {
        fadt = (fadt_t*)header;
        serial_printf(COM1, "ACPI: FADT loaded at %llx\n", (uint64_t)fadt);
        if (fadt->x_dsdt) {
            dsdt = (acpi_sdt_header_t*)P2V(fadt->x_dsdt);
        } else if (fadt->dsdt) {
            dsdt = (acpi_sdt_header_t*)P2V(fadt->dsdt);
        }
        if (dsdt) {
            serial_printf(COM1, "ACPI: DSDT loaded at %llx\n", (uint64_t)dsdt);
        }
    } else if (memcmp(header->signature, MADT_SIGNATURE, 4) == 0) {
        madt = (madt_t*)header;
        serial_printf(COM1, "ACPI: MADT loaded at %llx\n", (uint64_t)madt);
        acpi_parse_madt(madt);
    } else if (memcmp(header->signature, HPET_SIGNATURE, 4) == 0) {
        hpet = (hpet_t*)header;
        serial_printf(COM1, "ACPI: HPET loaded at %llx\n", (uint64_t)hpet);
    }
}

/**
 * Initializes the ACPI
 * @param rsdp_phys The physical address of the RSDP found by the MULTIBOOT2 info parser
 */
void acpi_init(phys_addr_t rsdp_phys) {
    if (!rsdp_phys) {
        serial_printf(COM1, "ACPI: bad RSDP address\n");
        return;
    }

    rsdp = (rsdp_t*)P2V(rsdp_phys);

    if (!acpi_verify_rsdp_checksum(rsdp)) {
        serial_printf(COM1, "ACPI: cannot verify RSDP checksum\n");
        return;
    }

    serial_printf(COM1, "ACPI: RSDP Revision %d, OEM %c%c%c%c%c%c found\n", rsdp->revision, rsdp->oem_id[0], rsdp->oem_id[1], rsdp->oem_id[2], rsdp->oem_id[3], rsdp->oem_id[4], rsdp->oem_id[5]);

    // If Revision >= 2 and xsdt_address is there, use XSDT
    if (rsdp->revision >= 2 && rsdp->xsdt_address != 0) {
        xsdt = (xsdt_t*)P2V(rsdp->xsdt_address);
        if (acpi_verify_sdt_checksum(&xsdt->header)) {
            serial_printf(COM1, "ACPI: Use 64-Bit XSDT at %llx\n", (uint64_t)xsdt);
            size_t entries = (xsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(uint64_t);
            for (size_t i = 0; i < entries; i++) {
                acpi_register_table((acpi_sdt_header_t*)xsdt->pointer_to_other_sdt[i]);
            }
            return;
        }
    }

    // RSDT is fallback
    if (rsdp->rsdt_address != 0) {
        rsdt = (rsdt_t*)P2V(rsdp->rsdt_address);
        if (acpi_verify_sdt_checksum(&rsdt->header)) {
            serial_printf(COM1, "ACPI: Use 32-Bit RSDT at %llx\n", (uint64_t)rsdt);
            size_t entries = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(uint32_t);
            for (size_t i = 0; i < entries; i++) {
                acpi_register_table((acpi_sdt_header_t*)(uint64_t)rsdt->pointer_to_other_sdt[i]);
            }
        }
    }
}

/**
 * Tries to power off the system via ACPI
 */
void acpi_power_off() {
    if (!fadt || !dsdt) {
        serial_printf(COM1, "ACPI: Cannot power off, FADT or DSDT are missing\n");
        return;
    }

    uint8_t* ptr = (uint8_t*)dsdt + sizeof(acpi_sdt_header_t);
    uint8_t* end = (uint8_t*)dsdt + dsdt->length;

    uint16_t SLP_TYPa = 0;
    uint16_t SLP_TYPb = 0;
    bool found = false;

    while (ptr < end - 8) {
        if (memcmp(ptr, "_S5_", 4) == 0) {
            ptr += 4;
            if (*ptr == 0x12) {
                ptr++;
                uint8_t b = *ptr++;
                uint8_t pkg_len_bytes = (b >> 6);
                ptr += pkg_len_bytes;

                ptr++;

                if (*ptr == 0x0A) ptr++;
                SLP_TYPa = *ptr++;

                if (*ptr == 0x0A) ptr++;
                SLP_TYPb = *ptr++;

                found = true;
                break;
            }
        }
        ptr++;
    }

    if (found) {
        serial_printf(COM1, "ACPI: Found _S5 package, SLP_TYPa: %x, SLP_TYPb: %x\n", SLP_TYPa, SLP_TYPb);

        outw(fadt->pm1a_cnt_blk, SLP_TYPa | (1 << 13)); // Bit 13: "Sleep Enable" (SLP_EN)
        if (fadt->pm1b_cnt_blk) {
            outw(fadt->pm1b_cnt_blk, SLP_TYPb | (1 << 13));
        }
    } else {
        serial_printf(COM1, "ACPI: _S5 package not found in DSDT\n");
    }
}