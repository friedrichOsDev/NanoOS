/**
 * @file acpi.c
 * @brief ACPI parsing and power management.
 * @author friedrichOsDev
 */

#include "acpi.h"
#include "../mm/mem_def.h"
#include "aml.h"
#include <arch/arch.h>
#include <lib/util/mem.h>
#include <lib/util/str.h>
#include <mm/memory.h>
#include <pch.h>

rsdp_t *rsdp = NULL;
rsdt_t *rsdt = NULL;
xsdt_t *xsdt = NULL;
fadt_t *fadt = NULL;
acpi_sdt_header_t *dsdt = NULL;
madt_t *madt = NULL;
madt_parsed_t madt_parsed;
hpet_t *hpet = NULL;
mcfg_t *mcfg = NULL;
mcfg_info_t mcfg_info;

/**
 * @brief Verifies the checksum of an ACPI RSDP structure.

 * @param[in] target_rsdp Pointer to the ACPI RSDP structure to verify.
 * @return true if checksum is valid, false otherwise.
 */
static bool acpi_verify_rsdp_checksum(rsdp_t *target_rsdp) {
    if (!target_rsdp || memcmp(target_rsdp->signature, RSDP_SIGNATURE, 8) != 0) {
        log_warn("rsdp checksum cannot be verified (invalid structure)");
        return false;
    }

    uint8_t *bytes = (uint8_t *)target_rsdp;
    uint8_t sum = 0;
    for (size_t i = 0; i < 20; i++) {
        sum = (uint8_t)(sum + bytes[i]);
    }

    if (sum != 0) {
        log_error("rsdp checksum is not valid");
        return false;
    }

    if (target_rsdp->revision >= 2) {
        uint8_t ext_sum = 0;
        for (size_t i = 0; i < target_rsdp->length; i++) {
            ext_sum = (uint8_t)(ext_sum + bytes[i]);
        }
        if (ext_sum != 0) {
            log_error("rsdp extended checksum is not valid");
            return false;
        }
    }
    return true;
}

/**
 * @brief Verifies the checksum of an ACPI table.
 *
 * @param[in] header Pointer to the ACPI table header to verify.
 * @return true if the checksum is valid, false otherwise.
 */
static bool acpi_verify_sdt_checksum(acpi_sdt_header_t *header) {
    if (!header)
        return false;
    uint8_t sum = 0;
    uint8_t *bytes = (uint8_t *)header;
    for (uint32_t i = 0; i < header->length; i++) {
        sum = (uint8_t)(sum + bytes[i]);
    }
    return sum == 0;
}

/**
 * @brief Parses the MADT table and stores the parsed information in a structure.
 *
 * @param[in] target_madt Pointer to the MADT table to parse.
 */
static void acpi_parse_madt(madt_t *target_madt) {
    if (!target_madt)
        return;

    memset(&madt_parsed, 0, sizeof(madt_parsed_t));
    uint8_t *ptr = target_madt->entries;
    uint8_t *end = (uint8_t *)target_madt + target_madt->header.length;

    while (ptr < end) {
        madt_entry_header_t *entry = (madt_entry_header_t *)ptr;
        if (entry->length == 0) {
            log_warn("skipping invalid MADT entry\n");
            break;
        }

        switch (entry->type) {
        case MADT_LAPIC_TYPE:
            if (madt_parsed.lapic_count == 0) {
                madt_parsed.lapics = (madt_lapic_entry_t *)entry;
            }
            madt_parsed.lapic_count++;
            break;
        case MADT_IOAPIC_TYPE:
            if (madt_parsed.ioapic_count == 0) {
                madt_parsed.ioapics = (madt_ioapic_entry_t *)entry;
            }
            madt_parsed.ioapic_count++;
            break;
        case MADT_ISO_TYPE:
            if (madt_parsed.iso_count == 0) {
                madt_parsed.isos = (madt_iso_entry_t *)entry;
            }
            madt_parsed.iso_count++;
            break;
        case MADT_IOAPIC_NMI_TYPE:
            if (madt_parsed.ioapic_nmi_count == 0) {
                madt_parsed.ioapic_nmis = (madt_ioapic_nmi_entry_t *)entry;
            }
            madt_parsed.ioapic_nmi_count++;
            break;
        case MADT_LAPIC_NMI_TYPE:
            if (madt_parsed.lapic_nmi_count == 0) {
                madt_parsed.lapic_nmis = (madt_lapic_nmi_entry_t *)entry;
            }
            madt_parsed.lapic_nmi_count++;
            break;
        case MADT_LAPIC_ADDRESS_OVERRIDE_TYPE:
            if (madt_parsed.lapic_override_count == 0) {
                madt_parsed.lapic_overrides = (madt_lapic_address_override_entry_t *)entry;
            }
            madt_parsed.lapic_override_count++;
            break;
        case MADT_LX2APIC_TYPE:
            if (madt_parsed.lx2apic_count == 0) {
                madt_parsed.lx2apics = (madt_lx2apic_entry_t *)entry;
            }
            madt_parsed.lx2apic_count++;
            break;
        default:
            break;
        }
        ptr += entry->length;
    }

    log_debug("MADT loaded (LAPICs: %d, IOAPICs: %d, ISOs: %d)", madt_parsed.lapic_count, madt_parsed.ioapic_count, madt_parsed.iso_count);
}

/**
 * @brief Registers an given ACPI table (e.g FADT, MADT, HPET, MCFG, etc.)
 *
 * @param[in] phys_header The physical address of the ACPI table.
 * @return E_OK on success, E_CHECKSUM on failure.
 */
static kerr_t acpi_register_table(acpi_sdt_header_t *phys_header) {
    acpi_sdt_header_t *header = (acpi_sdt_header_t *)P2V((uint64_t)phys_header);

    if (!acpi_verify_sdt_checksum(header)) {
        log_error("SDT header checksum invalid for: %c%c%c%c", header->signature[0], header->signature[1], header->signature[2], header->signature[3]);
        return E_CHECKSUM;
    }

    if (memcmp(header->signature, FADT_SIGNATURE, 4) == 0) {
        fadt = (fadt_t *)header;
        log_debug("FADT loaded at %llx", (uint64_t)fadt);
        if (fadt->x_dsdt) {
            dsdt = (acpi_sdt_header_t *)P2V(fadt->x_dsdt);
        } else if (fadt->dsdt) {
            dsdt = (acpi_sdt_header_t *)P2V(fadt->dsdt);
        }
        if (dsdt) {
            log_debug("DSDT loaded at %llx", (uint64_t)dsdt);
        } else {
            log_error("DSDT not found in FADT");
        }
    } else if (memcmp(header->signature, MADT_SIGNATURE, 4) == 0) {
        madt = (madt_t *)header;
        log_debug("MADT loaded at %llx", (uint64_t)madt);
        acpi_parse_madt(madt);
    } else if (memcmp(header->signature, HPET_SIGNATURE, 4) == 0) {
        hpet = (hpet_t *)header;
        log_debug("HPET loaded at %llx", (uint64_t)hpet);
    } else if (memcmp(header->signature, MCFG_SIGNATURE, 4) == 0) {
        mcfg = (mcfg_t *)header;
        log_debug("MCFG loaded at %llx", (uint64_t)mcfg);

        size_t header_size = sizeof(acpi_sdt_header_t) + sizeof(uint64_t);
        if (mcfg->header.length > header_size) {
            mcfg_info.count = (mcfg->header.length - header_size) / sizeof(mcfg_entry_t);
        } else {
            mcfg_info.count = 0;
        }

        if (mcfg_info.count > 0) {
            virt_addr_t mem;
            kerr_t err = kzalloc(mcfg_info.count * sizeof(mcfg_entry_runtime_t), &mem);
            if (k_failed(err)) {
                log_error("failed to allocate memory for mcfg_info.entries");
                mcfg_info.count = 0;
                mcfg_info.entries = nullptr;
                return err;
            }
            mcfg_info.entries = (mcfg_entry_runtime_t *)mem;

            for (size_t i = 0; i < mcfg_info.count; i++) {
                uint8_t start_bus = mcfg->entries[i].start_bus_number;
                uint8_t end_bus = mcfg->entries[i].end_bus_number;

                // 1 MB per Bus (32 Devs * 8 Funcs * 4096 Bytes ECAM Space)
                size_t bus_count = (size_t)(end_bus - start_bus + 1);
                size_t mmio_size = bus_count * 1024 * 1024;

                mcfg_info.entries[i].phys_base = mcfg->entries[i].base_address;
                err = vmm_map_mmio(vmm_kernel_space(), mcfg->entries[i].base_address, mmio_size, &(mcfg_info.entries[i].virt_base));
                if (k_failed(err)) {
                    log_error("failed to map MCFG[%zu]", i);
                    return err;
                }
                mcfg_info.entries[i].pci_segment = mcfg->entries[i].pci_segment_group;
                mcfg_info.entries[i].start_bus = start_bus;
                mcfg_info.entries[i].end_bus = end_bus;

                log_debug("MCFG[%zu] Seg %u, Bus %u-%u -> Phys: %llx, Virt: %llx", i, mcfg_info.entries[i].pci_segment, start_bus, end_bus, mcfg_info.entries[i].phys_base, mcfg_info.entries[i].virt_base);
            }
        }
    }
    return E_OK;
}

/** @brief Check if the ACPI mode is enabled, if not, enable it. */
static void acpi_enable_acpi_mode() {
    if (!fadt)
        return;

    uint16_t pm1_cnt = inw((uint16_t)fadt->pm1a_cnt_blk);
    if (pm1_cnt & 1) {
        return;
    }

    if (fadt->smi_cmd != 0 && fadt->acpi_enable != 0) {
        outb((uint16_t)fadt->smi_cmd, fadt->acpi_enable);

        int timeout = 30000;
        while (timeout--) {
            if (inw((uint16_t)fadt->pm1a_cnt_blk) & 1) {
                log_debug("ACPI mode enabled successfully");
                return;
            }
            arch_pause();
        }
        log_warn("failed to enable ACPI mode (timeout waiting for SCI_EN)");
    }
}

/**
 * @brief Initialize ACPI.
 *
 * @param[in] rsdp_phys The physical address of the RSDP.
 * @return E_OK on success, E_BADADDR or E_CHECKSUM on failure.
 */
kerr_t acpi_init(virt_addr_t rsdp_addr) {
    if (!rsdp_addr) {
        log_error("bad RSDP address");
        return E_BADADDR;
    }

    rsdp = (rsdp_t *)rsdp_addr;

    if (!acpi_verify_rsdp_checksum(rsdp)) {
        log_error("RSDP checksum invalid");
        return E_CHECKSUM;
    }

    log_debug("RSDP revision %d, OEM %c%c%c%c%c%c found", rsdp->revision, rsdp->oem_id[0], rsdp->oem_id[1], rsdp->oem_id[2], rsdp->oem_id[3], rsdp->oem_id[4], rsdp->oem_id[5]);

    if (rsdp->revision >= 2 && rsdp->xsdt_address != 0) {
        xsdt = (xsdt_t *)P2V(rsdp->xsdt_address);
        if (!acpi_verify_sdt_checksum((acpi_sdt_header_t *)xsdt)) {
            log_error("XSDT checksum invalid");
            return E_CHECKSUM;
        }
        log_debug("use 64-bit XSDT at %llx", (uint64_t)xsdt);
        size_t entries = (xsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(uint64_t);
        for (size_t i = 0; i < entries; i++) {
            acpi_register_table((acpi_sdt_header_t *)xsdt->pointer_to_other_sdt[i]);
        }
        acpi_enable_acpi_mode();
        return E_OK;
    }

    if (rsdp->rsdt_address != 0) {
        rsdt = (rsdt_t *)P2V(rsdp->rsdt_address);
        if (!acpi_verify_sdt_checksum(&rsdt->header)) {
            log_error("RSDT checksum invalid");
            return E_CHECKSUM;
        }
        log_debug("use 32-bit RSDT at %llx", (uint64_t)rsdt);
        size_t entries = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(uint32_t);
        for (size_t i = 0; i < entries; i++) {
            acpi_register_table((acpi_sdt_header_t *)(uint64_t)rsdt->pointer_to_other_sdt[i]);
        }
        acpi_enable_acpi_mode();
        return E_OK;
    }
    log_error("no valid XSDT or RSDT found in RSDP");
    return E_NOTFOUND;
}

/** @brief Power off the system. */
void acpi_power_off() {
    if (!fadt || !dsdt) {
        log_error("unable to power off, FADT or DSDT are missing");
        return;
    }

    s5_sleep_values_t s5;

    size_t dsdt_payload_len = dsdt->length - sizeof(acpi_sdt_header_t);
    const void *dsdt_payload = (const void *)((uint8_t *)dsdt + sizeof(acpi_sdt_header_t));

    if (aml_parse_s5(dsdt_payload, dsdt_payload_len, &s5) && s5.valid) {
        log_debug("successfully parsed _S5 package (SLP_TYPa: %x, SLP_TYPb: %x)", s5.slp_typa, s5.slp_typb);
        log_info("system shutdown");

        uint16_t pm1a_val = (uint16_t)(((s5.slp_typa & 7) << 10) | (1 << 13));
        outw((uint16_t)fadt->pm1a_cnt_blk, pm1a_val);

        if (fadt->pm1b_cnt_blk) {
            uint16_t pm1b_val = (uint16_t)(((s5.slp_typb & 7) << 10) | (1 << 13));
            outw((uint16_t)fadt->pm1b_cnt_blk, pm1b_val);
        }
    } else {
        log_warn("_S5 package not found or malformed in DSDT");
    }
}
