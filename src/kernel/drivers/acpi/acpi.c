/**
 * @file acpi.c
 * @author friedrichOsDev
 */

#include <acpi.h>
#include <string.h>
#include <serial.h>
#include <heap.h>
#include <vmm.h>
#include <pmm.h>

rsdp_t* rsdp;
rsdt_t* rsdt;
fadt_t* fadt;
madt_t* madt;
madt_parsed_t madt_parsed;

static virt_addr_t next_acpi_vaddr = VMM_ACPI_BASE;

/**
 * @brief Maps a physical memory region into the virtual ACPI memory space.
 * This is a simple linear allocator and does not handle freeing space.
 * @param phys_addr The starting physical address of the region to map.
 * @param length The length of the region.
 * @return The new virtual address of the mapped region, or NULL on failure.
 */
static void* acpi_map_permanent(phys_addr_t phys_addr, uint32_t length) {
    if (!phys_addr) {
        return NULL;
    }

    uint32_t offset = phys_addr & (VMM_PAGE_SIZE - 1);
    phys_addr_t phys_start = phys_addr - offset;
    uint32_t num_pages = (offset + length + VMM_PAGE_SIZE - 1) / VMM_PAGE_SIZE;

    if (next_acpi_vaddr + (num_pages * VMM_PAGE_SIZE) > VMM_ACPI_END) {
        serial_printf("ACPI: Out of virtual memory for mapping tables!\n");
        return NULL;
    }

    virt_addr_t virt_start = next_acpi_vaddr;
    
    vmm_map_pages(vmm_get_page_directory(), virt_start, phys_start, VMM_PAGE_PRESENT | VMM_PAGE_READ_WRITE, num_pages);

    next_acpi_vaddr += num_pages * VMM_PAGE_SIZE;

    return (void*)(virt_start + offset);
}

/**
 * @brief Verifies the checksum of the Root System Description Pointer (RSDP).
 * @param rsdp Pointer to the RSDP structure to verify.
 * @return true if the checksum is valid, false otherwise.
 */
static bool acpi_verify_rsdp_checksum(rsdp_t* rsdp) {
    uint8_t* bytes = (uint8_t*)rsdp;
    uint8_t sum = 0;
    for (size_t i = 0; i < 20; i++) {
        sum += bytes[i];
    }

    if (sum != 0) return false;

    if (rsdp->revision > 0) {
        sum = 0;
        for (size_t i = 0; i < sizeof(rsdp_t); i++) {
            sum += bytes[i];
        }
        return sum == 0;
    }

    return true;
}

/**
 * @brief Verifies the checksum of an ACPI System Description Table (SDT).
 * @param header Pointer to the SDT header to verify.
 * @return true if the checksum is valid, false otherwise.
 */
static bool acpi_verify_sdt_checksum(acpi_sdt_header_t* header) {
    uint8_t sum = 0;
    uint8_t* bytes = (uint8_t*)header;
    for (size_t i = 0; i < header->length; i++) {
        sum += bytes[i];
    }
    return sum == 0;
}

/**
 * @brief Searches for an ACPI table with the given signature in the RSDT.
 * @param signature The 4-character signature of the table to find.
 * @return Pointer to the table header if found and verified, NULL otherwise.
 */
static acpi_sdt_header_t* acpi_find_table(const char* signature) {
    if (!rsdt) return NULL;

    size_t entries = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / 4;
    for (size_t i = 0; i < entries; i++) {
        phys_addr_t table_phys = rsdt->pointer_to_other_sdt[i];

        vmm_prepare_zero_window(PMM_ALIGN_DOWN(table_phys), 0);
        acpi_sdt_header_t* header = (acpi_sdt_header_t*)(VMM_ZERO_WINDOW + (table_phys & (VMM_PAGE_SIZE - 1)));

        if (memcmp(header->signature, signature, 4) == 0) {
            uint32_t length = header->length;
            acpi_sdt_header_t* full_table = acpi_map_permanent(table_phys, length);

            if (full_table && acpi_verify_sdt_checksum(full_table)) {
                return full_table;
            } else {
                serial_printf("ACPI: Table %s found but checksum failed!\n", signature);
            }
        }
    }
    return NULL;
}

/**
 * @brief Locates, maps, and verifies the Root System Description Table (RSDT).
 * Uses the address provided in the RSDP.
 */
static void acpi_find_rsdt() {
    phys_addr_t rsdt_phys = rsdp->rsdt_address;
    
    vmm_prepare_zero_window(PMM_ALIGN_DOWN(rsdt_phys), 0);
    acpi_sdt_header_t* rsdt_header = (acpi_sdt_header_t*)(VMM_ZERO_WINDOW + (rsdt_phys & (VMM_PAGE_SIZE - 1)));
    uint32_t rsdt_length = rsdt_header->length;

    rsdt = acpi_map_permanent(rsdt_phys, rsdt_length);
    
    if (rsdt) {
        serial_printf("ACPI: RSDT found at phys %x, mapped to %x\n", rsdt_phys, (uint32_t)rsdt);
        if (!acpi_verify_sdt_checksum(&rsdt->header)) {
            serial_printf("ACPI: RSDT checksum verification failed!\n");
            rsdt = NULL;
        }
    }
}

/**
 * @brief Locates the Fixed ACPI Description Table (FADT) in the RSDT.
 * If found, sets the global fadt pointer.
 */
static void acpi_find_fadt() {
    fadt = (fadt_t*)acpi_find_table(FADT_SIGNATURE);
    if (fadt) {
        serial_printf("ACPI: FADT found, mapped to %x\n", (uint32_t)fadt);
    } else {
        serial_printf("ACPI: FADT not found!\n");
    }
}

/**
 * @brief Locates the Multiple APIC Description Table (MADT) in the RSDT.
 * If found, sets the global madt pointer.
 */
static void acpi_find_madt() {
    madt = (madt_t*)acpi_find_table(MADT_SIGNATURE);
    if (madt) {
        serial_printf("ACPI: MADT found, mapped to %x\n", (uint32_t)madt);
    } else {
        serial_printf("ACPI: MADT not found!\n");
    }
}

/**
 * @brief Parses the MADT and populates the madt_parsed structure.
 * Allocates memory for the various APIC entry types found in the table.
 */
static void acpi_parse_madt() {
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

    if (!madt) return;

    uint8_t* ptr = (uint8_t*)(madt) + sizeof(madt_t);
    uint8_t* end = (uint8_t*)(madt) + madt->header.length;

    while (ptr < end) {
        madt_entry_header_t* entry = (madt_entry_header_t*)ptr;
        switch (entry->type) {
            case MADT_LAPIC_TYPE:
                madt_parsed.lapic_count++;
                break;
            case MADT_IOAPIC_TYPE:
                madt_parsed.ioapic_count++;
                break;
            case MADT_ISO_TYPE:
                madt_parsed.iso_count++;
                break;
            case MADT_IOAPIC_NMI_TYPE:
                madt_parsed.ioapic_nmi_count++;
                break;
            case MADT_LAPIC_NMI_TYPE:
                madt_parsed.lapic_nmi_count++;
                break;
            case MADT_LAPIC_ADDRESS_OVERRIDE_TYPE:
                madt_parsed.lapic_address_override_count++;
                break;
            case MADT_LX2APIC_TYPE:
                madt_parsed.lx2apic_count++;
                break;
            default:
                break;
        }
        ptr += entry->length;
    }

    serial_printf("ACPI: Debug: Allocating memory for MADT entries\n");
    if (madt_parsed.lapic_count > 0) madt_parsed.lapics = (madt_lapic_entry_t*)kzalloc(madt_parsed.lapic_count * sizeof(madt_lapic_entry_t));
    if (madt_parsed.ioapic_count > 0) madt_parsed.ioapics = (madt_ioapic_entry_t*)kzalloc(madt_parsed.ioapic_count * sizeof(madt_ioapic_entry_t));
    if (madt_parsed.iso_count > 0) madt_parsed.isos = (madt_iso_entry_t*)kzalloc(madt_parsed.iso_count * sizeof(madt_iso_entry_t));
    if (madt_parsed.ioapic_nmi_count > 0) madt_parsed.ioapic_nmis = (madt_ioapic_nmi_entry_t*)kzalloc(madt_parsed.ioapic_nmi_count * sizeof(madt_ioapic_nmi_entry_t));
    if (madt_parsed.lapic_nmi_count > 0) madt_parsed.lapic_nmis = (madt_lapic_nmi_entry_t*)kzalloc(madt_parsed.lapic_nmi_count * sizeof(madt_lapic_nmi_entry_t));
    if (madt_parsed.lapic_address_override_count > 0) madt_parsed.lapic_address_overrides = (madt_lapic_address_override_entry_t*)kzalloc(madt_parsed.lapic_address_override_count * sizeof(madt_lapic_address_override_entry_t));
    if (madt_parsed.lx2apic_count > 0) madt_parsed.lx2apics = (madt_lx2apic_entry_t*)kzalloc(madt_parsed.lx2apic_count * sizeof(madt_lx2apic_entry_t));
    serial_printf("ACPI: Debug: Memory allocated for MADT entries\n");

    size_t lapic_idx = 0, ioapic_idx = 0, iso_idx = 0, ioapic_nmi_idx = 0, lapic_nmi_idx = 0, lapic_address_override_idx = 0, lx2apic_idx = 0;

    ptr = (uint8_t*)(madt) + sizeof(madt_t);
    while (ptr < end) {
        madt_entry_header_t* entry = (madt_entry_header_t*)ptr;
        switch (entry->type) {
            case MADT_LAPIC_TYPE:
                memcpy(&madt_parsed.lapics[lapic_idx++], entry, entry->length);
                break;
            case MADT_IOAPIC_TYPE:
                memcpy(&madt_parsed.ioapics[ioapic_idx++], entry, entry->length);
                break;
            case MADT_ISO_TYPE:
                memcpy(&madt_parsed.isos[iso_idx++], entry, entry->length);
                break;
            case MADT_IOAPIC_NMI_TYPE:
                memcpy(&madt_parsed.ioapic_nmis[ioapic_nmi_idx++], entry, entry->length);
                break;
            case MADT_LAPIC_NMI_TYPE:
                memcpy(&madt_parsed.lapic_nmis[lapic_nmi_idx++], entry, entry->length);
                break;
            case MADT_LAPIC_ADDRESS_OVERRIDE_TYPE:
                memcpy(&madt_parsed.lapic_address_overrides[lapic_address_override_idx++], entry, entry->length);
                break;
            case MADT_LX2APIC_TYPE:
                memcpy(&madt_parsed.lx2apics[lx2apic_idx++], entry, entry->length);
                break;
            default:
                break;
        }
        ptr += entry->length;
    }
}

/**
 * @brief Initializes the ACPI subsystem.
 */
void acpi_init(void) {
    serial_printf("ACPI: start\n");
    if (!rsdp) {
        serial_printf("ACPI: RSDP not found in multiboot tags!\n");
        return;
    }

    rsdp_t* rsdp_new = (rsdp_t*)acpi_map_permanent((phys_addr_t)rsdp, sizeof(rsdp_t));
    if (!rsdp_new) {
        serial_printf("ACPI: Failed to map RSDP into ACPI virtual space!\n");
        return;
    }
    rsdp = rsdp_new;

    if (!acpi_verify_rsdp_checksum(rsdp)) {
        serial_printf("ACPI: RSDP checksum verification failed!\n");
        rsdp = NULL;
        return;
    }
    serial_printf("ACPI: RSDP verified\n");

    serial_printf("ACPI: Finding tables...\n");
    acpi_find_rsdt();
    acpi_find_fadt();
    acpi_find_madt();
    acpi_parse_madt();
    serial_printf("ACPI: done\n");
}
