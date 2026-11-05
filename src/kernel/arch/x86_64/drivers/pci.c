/**
 * @file pci.c
 * @brief Basic PCI scanning and driver registration.
 * @author friedrichOsDev
 */

#include "../acpi/acpi.h"
#include "_pci.h"
#include <arch/arch.h>
#include <drivers/pci.h>
#include <kernel/sync.h>
#include <lib/util/str.h>
#include <mm/memory.h>

static pci_device_t *device_list_head = NULL;
static pci_driver_t *driver_list_head = NULL;
static spinlock_t pci_lock = SPINLOCK_INIT;

/**
 * @brief Computes the virtual ECAM memory address for a given PCI device configuration space offset.
 * 
 * @param[in] bus PCI bus number.
 * @param[in] dev PCI device number.
 * @param[in] func PCI function number.
 * @param[in] offset Configuration space register offset.
 * @return Virtual address corresponding to the ECAM location, or 0 if not found.
 */
static virt_addr_t pci_get_ecam_addr(uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset) {
    for (size_t i = 0; i < mcfg_info.count; i++) {
        mcfg_entry_runtime_t *entry = &mcfg_info.entries[i];
        if (bus >= entry->start_bus && bus <= entry->end_bus) {
            uint64_t relative_bus = (uint64_t)(bus - entry->start_bus);
            uint64_t dev_offset = ((relative_bus << 20) | ((uint64_t)dev << 15) | ((uint64_t)func << 12) | (offset & 0xFFF));
            return entry->virt_base + dev_offset;
        }
    }
    return 0;
}

/**
 * @brief Reads an 8-bit value from a PCI device's configuration space.
 * 
 * @param[in] dev Pointer to the target PCI device structure.
 * @param[in] offset Register offset in configuration space.
 * @return 8-bit value read from the register, or 0xFF on failure.
 */
uint8_t pci_read8(pci_device_t *dev, uint16_t offset) {
    if (!dev || !dev->config_space_virt)
        return 0xFF;
    return *(volatile uint8_t *)(dev->config_space_virt + offset);
}

/**
 * @brief Reads a 16-bit value from a PCI device's configuration space.
 * 
 * @param[in] dev Pointer to the target PCI device structure.
 * @param[in] offset Register offset in configuration space.
 * @return 16-bit value read from the register, or 0xFFFF on failure.
 */
uint16_t pci_read16(pci_device_t *dev, uint16_t offset) {
    if (!dev || !dev->config_space_virt)
        return 0xFFFF;
    return *(volatile uint16_t *)(dev->config_space_virt + offset);
}

/**
 * @brief Reads a 32-bit value from a PCI device's configuration space.
 * 
 * @param[in] dev Pointer to the target PCI device structure.
 * @param[in] offset Register offset in configuration space.
 * @return 32-bit value read from the register, or 0xFFFFFFFF on failure.
 */
uint32_t pci_read32(pci_device_t *dev, uint16_t offset) {
    if (!dev || !dev->config_space_virt)
        return 0xFFFFFFFF;
    return *(volatile uint32_t *)(dev->config_space_virt + offset);
}

/**
 * @brief Writes an 8-bit value to a PCI device's configuration space.
 * 
 * @param[in] dev Pointer to the target PCI device structure.
 * @param[in] offset Register offset in configuration space.
 * @param[in] val 8-bit value to write.
 */
void pci_write8(pci_device_t *dev, uint16_t offset, uint8_t val) {
    if (!dev || !dev->config_space_virt)
        return;
    *(volatile uint8_t *)(dev->config_space_virt + offset) = val;
}

/**
 * @brief Writes a 16-bit value to a PCI device's configuration space.
 * 
 * @param[in] dev Pointer to the target PCI device structure.
 * @param[in] offset Register offset in configuration space.
 * @param[in] val 16-bit value to write.
 */
void pci_write16(pci_device_t *dev, uint16_t offset, uint16_t val) {
    if (!dev || !dev->config_space_virt)
        return;
    *(volatile uint16_t *)(dev->config_space_virt + offset) = val;
}

/**
 * @brief Writes a 32-bit value to a PCI device's configuration space.
 * 
 * @param[in] dev Pointer to the target PCI device structure.
 * @param[in] offset Register offset in configuration space.
 * @param[in] val 32-bit value to write.
 */
void pci_write32(pci_device_t *dev, uint16_t offset, uint32_t val) {
    if (!dev || !dev->config_space_virt)
        return;
    *(volatile uint32_t *)(dev->config_space_virt + offset) = val;
}

/**
 * @brief Parses and configures Base Address Registers (BARs) for a PCI device.
 * 
 * @param[in,out] dev Pointer to the PCI device structure to populate.
 */
static void pci_parse_bars(pci_device_t *dev) {
    for (int i = 0; i < 6; i++) {
        uint16_t bar_offset = (uint16_t)(0x10 + (i * 4));
        uint32_t bar_low = pci_read32(dev, bar_offset);

        if (!bar_low)
            continue;

        bool is_io = (bar_low & 0x01) != 0;
        dev->is_mmio_bar[i] = !is_io;

        if (is_io) {
            dev->bar[i] = bar_low & ~0x3U;
            dev->is_64bit_bar[i] = false;

            pci_write32(dev, bar_offset, 0xFFFFFFFF);
            uint32_t size_mask = pci_read32(dev, bar_offset);
            pci_write32(dev, bar_offset, bar_low);

            dev->bar_size[i] = ~(size_mask & ~0x3U) + 1;
        } else {
            bool is_64 = ((bar_low >> 1) & 0x03) == 0x02;
            dev->is_64bit_bar[i] = is_64;

            uint64_t full_bar = bar_low & ~0x0FULL;

            if (is_64 && i < 5) {
                uint32_t bar_high = pci_read32(dev, (uint16_t)(bar_offset + 4));
                full_bar |= ((uint64_t)bar_high << 32);

                pci_write32(dev, bar_offset, 0xFFFFFFFF);
                pci_write32(dev, (uint16_t)(bar_offset + 4), 0xFFFFFFFF);
                uint32_t mask_low = pci_read32(dev, bar_offset);
                uint32_t mask_high = pci_read32(dev, (uint16_t)(bar_offset + 4));
                pci_write32(dev, bar_offset, bar_low);
                pci_write32(dev, (uint16_t)(bar_offset + 4), bar_high);

                uint64_t mask = ((uint64_t)mask_high << 32) | (mask_low & ~0x0FULL);
                dev->bar_size[i] = ~mask + 1;
            } else {
                pci_write32(dev, bar_offset, 0xFFFFFFFF);
                uint32_t mask_low = pci_read32(dev, bar_offset);
                pci_write32(dev, bar_offset, bar_low);

                dev->bar_size[i] = ~(mask_low & ~0x0FULL) + 1;
            }

            dev->bar[i] = full_bar;

            if (is_64)
                i++;
        }
    }
}

/**
 * @brief Scans and parses capabilities list (such as MSI/MSI-X) for a PCI device.
 * 
 * @param[in,out] dev Pointer to the PCI device structure to update.
 */
static void pci_parse_capabilities(pci_device_t *dev) {
    uint16_t status = pci_read16(dev, 0x06);
    if (!(status & (1 << 4)))
        return;

    uint8_t cap_ptr = (uint8_t)(pci_read8(dev, 0x34) & ~0x03U);

    while (cap_ptr >= 0x40 && cap_ptr < 0xFF) {
        uint8_t cap_id = pci_read8(dev, cap_ptr);
        uint8_t next_ptr = (uint8_t)(pci_read8(dev, (uint16_t)(cap_ptr + 1)) & ~0x03U);

        if (cap_id == PCI_CAP_ID_MSI) {
            dev->msi_cap_offset = cap_ptr;
        } else if (cap_id == PCI_CAP_ID_MSIX) {
            dev->msix_cap_offset = cap_ptr;
        }

        if (next_ptr == cap_ptr)
            break;
        cap_ptr = next_ptr;
    }
}

/**
 * @brief Probes a specific PCI bus, device, and function slot, registering it if present.
 * 
 * @param[in] bus PCI bus number.
 * @param[in] dev PCI device number.
 * @param[in] func PCI function number.
 * @return E_OK on success, or an appropriate error code on failure.
 */
static kerr_t pci_probe_device(uint8_t bus, uint8_t dev, uint8_t func) {
    virt_addr_t ecam = pci_get_ecam_addr(bus, dev, func, 0);
    if (!ecam)
        return E_INVAL;

    volatile uint16_t *vendor_ptr = (volatile uint16_t *)ecam;
    if (*vendor_ptr == 0xFFFF)
        return E_NOTFOUND;

    virt_addr_t mem;
    kerr_t err = kzalloc(sizeof(pci_device_t), &mem);
    if (k_failed(err)) {
        log_error("failed to allocate device structure");
        return err;
    }
    pci_device_t *device = (pci_device_t *)mem;

    device->bus = bus;
    device->device = dev;
    device->function = func;
    device->config_space_virt = ecam;

    device->vendor_id = pci_read16(device, 0x00);
    device->device_id = pci_read16(device, 0x02);
    device->revision_id = pci_read8(device, 0x08);
    device->prog_if = pci_read8(device, 0x09);
    device->subclass = pci_read8(device, 0x0A);
    device->class_code = pci_read8(device, 0x0B);

    pci_parse_bars(device);
    pci_parse_capabilities(device);

    irq_flags_t flags = spinlock_acquire_irqsave(&pci_lock);
    device->next = device_list_head;
    device_list_head = device;
    spinlock_release_irqrestore(&pci_lock, flags);

    log_debug("PCI device [%02x:%02x.%d] %04x:%04x (Class %02x, Sub %02x)", bus, dev, func, device->vendor_id, device->device_id, device->class_code, device->subclass);
    return E_OK;
}

/**
 * @brief Initializes the PCI subsystem by iterating over all MCFG entries and probing available devices.
 * 
 * @return E_OK on success, or an appropriate error code on failure.
 */
kerr_t pci_init() {
    if (mcfg_info.count == 0 || !mcfg_info.entries) {
        log_warn("no MCFG entries available, PCI cannot be initialized");
        return E_NOINIT;
    }

    for (size_t i = 0; i < mcfg_info.count; i++) {
        mcfg_entry_runtime_t *entry = &mcfg_info.entries[i];
        for (uint32_t bus = entry->start_bus; bus <= entry->end_bus; bus++) {
            for (uint8_t dev = 0; dev < 32; dev++) {
                virt_addr_t base_ecam = pci_get_ecam_addr((uint8_t)bus, dev, 0, 0);
                if (!base_ecam)
                    continue;

                if (*(volatile uint16_t *)base_ecam == 0xFFFF)
                    continue;

                uint8_t header_type = *(volatile uint8_t *)(base_ecam + 0x0E);
                bool is_multi_function = (header_type & 0x80) != 0;

                uint8_t max_func = is_multi_function ? 8 : 1;
                for (uint8_t func = 0; func < max_func; func++) {
                    kerr_t err = pci_probe_device((uint8_t)bus, dev, func);
                    if (err == E_NOTFOUND)
                        continue; // no problem, slot just doesn't exist
                    if (k_failed(err))
                        return err;
                }
            }
        }
    }
    return E_OK;
}

/**
 * @brief Retrieves the head pointer of the detected PCI device linked list.
 * 
 * @return Pointer to the first pci_device_t element in the list.
 */
pci_device_t *pci_get_devices() {
    return device_list_head;
}

/**
 * @brief Attempts to match a newly registered driver against all discovered PCI devices.
 * 
 * @param[in] driver Pointer to the PCI driver structure to match.
 * @return E_OK if a device successfully bound, or an error code otherwise.
 */
static kerr_t pci_match_driver(pci_driver_t *driver) {
    irq_flags_t flags = spinlock_acquire_irqsave(&pci_lock);
    kerr_t err = E_NOTFOUND;
    pci_device_t *dev = device_list_head;
    while (dev) {
        bool match = true;

        if (driver->vendor_id != 0xFFFF && driver->vendor_id != dev->vendor_id)
            match = false;
        if (driver->device_id != 0xFFFF && driver->device_id != dev->device_id)
            match = false;
        if (driver->class_code != 0xFF && driver->class_code != dev->class_code)
            match = false;
        if (driver->subclass != 0xFF && driver->subclass != dev->subclass)
            match = false;

        if (match && driver->probe) {
            if (driver->probe(dev)) {
                log_debug("PCI driver '%s' bound to [%02x:%02x.%d]", driver->name, dev->bus, dev->device, dev->function);
                err = E_OK;
            } else {
                err = E_NOPROBE;
            }
        }
        dev = dev->next;
    }
    spinlock_release_irqrestore(&pci_lock, flags);
    return err;
}

/**
 * @brief Registers a new PCI driver and triggers matching against existing devices.
 * 
 * @param[in] driver Pointer to the PCI driver structure to register.
 * @return E_OK or binding status result code.
 */
kerr_t pci_register_driver(pci_driver_t *driver) {
    if (!driver)
        return E_INVAL;

    irq_flags_t flags = spinlock_acquire_irqsave(&pci_lock);
    driver->next = driver_list_head;
    driver_list_head = driver;
    spinlock_release_irqrestore(&pci_lock, flags);

    return pci_match_driver(driver);
}

/**
 * @brief Enables Message Signaled Interrupts (MSI) for a specified PCI device.
 * 
 * @param[in,out] dev Pointer to the PCI device.
 * @param[in] vector Interrupt vector number to trigger.
 * @param[in] lapic_id Target Local APIC ID.
 * @return true on success, false if MSI capability is missing or invalid.
 */
bool pci_enable_msi(pci_device_t *dev, uint8_t vector, uint8_t lapic_id) {
    if (!dev || dev->msi_cap_offset == 0)
        return false;

    uint8_t cap = dev->msi_cap_offset;
    uint16_t msg_ctrl = pci_read16(dev, (uint16_t)(cap + 0x02));
    bool is_64bit = (msg_ctrl & (1 << 7)) != 0;

    // x86 MSI Address Layout: 0xFEE00000 | (Destination ID << 12)
    uint32_t msg_addr_low = 0xFEE00000 | ((uint32_t)lapic_id << 12);
    uint32_t msg_data = vector; // Delivery mode 000 (Fixed), Edge triggered

    pci_write32(dev, (uint16_t)(cap + 0x04), msg_addr_low);

    if (is_64bit) {
        pci_write32(dev, (uint16_t)(cap + 0x08), 0x00000000); // Address High
        pci_write16(dev, (uint16_t)(cap + 0x0C), (uint16_t)msg_data);
    } else {
        pci_write16(dev, (uint16_t)(cap + 0x08), (uint16_t)msg_data);
    }

    uint16_t cmd = pci_read16(dev, 0x04);
    pci_write16(dev, 0x04, cmd | (1 << 2)); // Bit 2: Bus Master Enable

    msg_ctrl |= (1 << 0);
    pci_write16(dev, (uint16_t)(cap + 0x02), msg_ctrl);

    log_debug("enabled MSI on [%02x:%02x.%d] -> vector %u, LAPIC %u", dev->bus, dev->device, dev->function, vector, lapic_id);

    return true;
}
