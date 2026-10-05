/**
 * @file pci.h
 * @brief PCIe / PCI MMCONFIG Subsystem & Driver Framework.
 * @author friedrichOsDev
 */

#include <core/sync.h>
#include <arch/x86_64/drivers/pci.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/mm/heap.h>
#include <lib/string.h>

static pci_device_t *device_list_head = NULL;
static pci_driver_t *driver_list_head = NULL;
static spinlock_t pci_lock = SPINLOCK_INIT;

/**
 * @brief Berechnet die virtuelle Adresse des Konfigurationsraums für ein PCI-Gerät via ECAM (MCFG).
 */
static virt_addr_t pci_get_ecam_addr(uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset) {
    for (size_t i = 0; i < mcfg_info.count; i++) {
        mcfg_entry_runtime_t *entry = &mcfg_info.entries[i];
        if (bus >= entry->start_bus && bus <= entry->end_bus) {
            uint64_t relative_bus = bus - entry->start_bus;
            uint64_t dev_offset = ((relative_bus << 20) | ((uint64_t)dev << 15) | ((uint64_t)func << 12) | (offset & 0xFFF));
            return entry->virt_base + dev_offset;
        }
    }
    return 0;
}

uint8_t pci_read8(pci_device_t *dev, uint16_t offset) {
    if (!dev || !dev->config_space_virt) return 0xFF;
    return *(volatile uint8_t *)(dev->config_space_virt + offset);
}

uint16_t pci_read16(pci_device_t *dev, uint16_t offset) {
    if (!dev || !dev->config_space_virt) return 0xFFFF;
    return *(volatile uint16_t *)(dev->config_space_virt + offset);
}

uint32_t pci_read32(pci_device_t *dev, uint16_t offset) {
    if (!dev || !dev->config_space_virt) return 0xFFFFFFFF;
    return *(volatile uint32_t *)(dev->config_space_virt + offset);
}

void pci_write8(pci_device_t *dev, uint16_t offset, uint8_t val) {
    if (!dev || !dev->config_space_virt) return;
    *(volatile uint8_t *)(dev->config_space_virt + offset) = val;
}

void pci_write16(pci_device_t *dev, uint16_t offset, uint16_t val) {
    if (!dev || !dev->config_space_virt) return;
    *(volatile uint16_t *)(dev->config_space_virt + offset) = val;
}

void pci_write32(pci_device_t *dev, uint16_t offset, uint32_t val) {
    if (!dev || !dev->config_space_virt) return;
    *(volatile uint32_t *)(dev->config_space_virt + offset) = val;
}

/**
 * @brief Liest BARs aus und bestimmt Speicheradresse, Typ (32/64-Bit, MMIO/IO) und Größe.
 */
static void pci_parse_bars(pci_device_t *dev) {
    for (int i = 0; i < 6; i++) {
        uint16_t bar_offset = 0x10 + (i * 4);
        uint32_t bar_low = pci_read32(dev, bar_offset);

        if (!bar_low) continue;

        bool is_io = (bar_low & 0x01) != 0;
        dev->is_mmio_bar[i] = !is_io;

        if (is_io) {
            dev->bar[i] = bar_low & ~0x3;
            dev->is_64bit_bar[i] = false;

            pci_write32(dev, bar_offset, 0xFFFFFFFF);
            uint32_t size_mask = pci_read32(dev, bar_offset);
            pci_write32(dev, bar_offset, bar_low);

            dev->bar_size[i] = ~(size_mask & ~0x3) + 1;
        } else {
            bool is_64 = ((bar_low >> 1) & 0x03) == 0x02;
            dev->is_64bit_bar[i] = is_64;

            uint64_t full_bar = bar_low & ~0x0FULL;

            if (is_64 && i < 5) {
                uint32_t bar_high = pci_read32(dev, bar_offset + 4);
                full_bar |= ((uint64_t)bar_high << 32);

                pci_write32(dev, bar_offset, 0xFFFFFFFF);
                pci_write32(dev, bar_offset + 4, 0xFFFFFFFF);
                uint32_t mask_low = pci_read32(dev, bar_offset);
                uint32_t mask_high = pci_read32(dev, bar_offset + 4);
                pci_write32(dev, bar_offset, bar_low);
                pci_write32(dev, bar_offset + 4, bar_high);

                uint64_t mask = ((uint64_t)mask_high << 32) | (mask_low & ~0x0FULL);
                dev->bar_size[i] = ~mask + 1;
            } else {
                pci_write32(dev, bar_offset, 0xFFFFFFFF);
                uint32_t mask_low = pci_read32(dev, bar_offset);
                pci_write32(dev, bar_offset, bar_low);

                dev->bar_size[i] = ~(mask_low & ~0x0FULL) + 1;
            }

            dev->bar[i] = full_bar;

            if (is_64) i++; // Nächste BAR wird von den oberen 32 Bit belegt
        }
    }
}

/**
 * @brief Parst die Capability Linked List im Konfigurationsraum (MSI/MSI-X Offsets).
 */
static void pci_parse_capabilities(pci_device_t *dev) {
    uint16_t status = pci_read16(dev, 0x06);
    if (!(status & (1 << 4))) return; // Bit 4: Capabilities List implementiert?

    uint8_t cap_ptr = pci_read8(dev, 0x34) & ~0x03;

    while (cap_ptr >= 0x40 && cap_ptr < 0xFF) {
        uint8_t cap_id = pci_read8(dev, cap_ptr);
        uint8_t next_ptr = pci_read8(dev, cap_ptr + 1) & ~0x03;

        if (cap_id == PCI_CAP_ID_MSI) {
            dev->msi_cap_offset = cap_ptr;
        } else if (cap_id == PCI_CAP_ID_MSIX) {
            dev->msix_cap_offset = cap_ptr;
        }

        if (next_ptr == cap_ptr) break;
        cap_ptr = next_ptr;
    }
}

static void pci_probe_device(uint8_t bus, uint8_t dev, uint8_t func) {
    virt_addr_t ecam = pci_get_ecam_addr(bus, dev, func, 0);
    if (!ecam) return;

    volatile uint16_t *vendor_ptr = (volatile uint16_t *)ecam;
    if (*vendor_ptr == 0xFFFF) return;

    pci_device_t *device = (pci_device_t *)kzalloc(sizeof(pci_device_t));
    if (!device) return;

    device->bus = bus;
    device->device = dev;
    device->function = func;
    device->config_space_virt = ecam;

    device->vendor_id   = pci_read16(device, 0x00);
    device->device_id   = pci_read16(device, 0x02);
    device->revision_id = pci_read8(device, 0x08);
    device->prog_if     = pci_read8(device, 0x09);
    device->subclass    = pci_read8(device, 0x0A);
    device->class_code  = pci_read8(device, 0x0B);

    pci_parse_bars(device);
    pci_parse_capabilities(device);

    uint64_t flags = spinlock_acquire_irqsave(&pci_lock);
    device->next = device_list_head;
    device_list_head = device;
    spinlock_release_irqrestore(&pci_lock, flags);

    serial_printf(COM1, "PCI: Device [%02x:%02x.%d] %04x:%04x (Class %02x, Sub %02x)\n", bus, dev, func, device->vendor_id, device->device_id, device->class_code, device->subclass);
}

void pci_init() {
    if (mcfg_info.count == 0 || !mcfg_info.entries) {
        serial_printf(COM1, "PCI: No MCFG entries available\n");
        return;
    }

    serial_printf(COM1, "PCI: Initializing ECAM bus scan via MCFG...\n");

    for (size_t i = 0; i < mcfg_info.count; i++) {
        mcfg_entry_runtime_t *entry = &mcfg_info.entries[i];
        for (uint32_t bus = entry->start_bus; bus <= entry->end_bus; bus++) {
            for (uint8_t dev = 0; dev < 32; dev++) {
                virt_addr_t base_ecam = pci_get_ecam_addr(bus, dev, 0, 0);
                if (!base_ecam) continue;

                if (*(volatile uint16_t *)base_ecam == 0xFFFF) continue;

                uint8_t header_type = *(volatile uint8_t *)(base_ecam + 0x0E);
                bool is_multi_function = (header_type & 0x80) != 0;

                uint8_t max_func = is_multi_function ? 8 : 1;
                for (uint8_t func = 0; func < max_func; func++) {
                    pci_probe_device(bus, dev, func);
                }
            }
        }
    }
}

pci_device_t *pci_get_devices() {
    return device_list_head;
}

static void pci_match_driver(pci_driver_t *driver) {
    uint64_t flags = spinlock_acquire_irqsave(&pci_lock);
    pci_device_t *dev = device_list_head;
    while (dev) {
        bool match = true;

        if (driver->vendor_id != 0xFFFF && driver->vendor_id != dev->vendor_id) match = false;
        if (driver->device_id != 0xFFFF && driver->device_id != dev->device_id) match = false;
        if (driver->class_code != 0xFF && driver->class_code != dev->class_code) match = false;
        if (driver->subclass != 0xFF && driver->subclass != dev->subclass) match = false;

        if (match && driver->probe) {
            if (driver->probe(dev)) {
                serial_printf(COM1, "PCI: Driver '%s' bound to [%02x:%02x.%d]\n", driver->name, dev->bus, dev->device, dev->function);
            }
        }
        dev = dev->next;
    }
    spinlock_release_irqrestore(&pci_lock, flags);
}

void pci_register_driver(pci_driver_t *driver) {
    if (!driver) return;

    uint64_t flags = spinlock_acquire_irqsave(&pci_lock);
    driver->next = driver_list_head;
    driver_list_head = driver;
    spinlock_release_irqrestore(&pci_lock, flags);

    pci_match_driver(driver);
}

bool pci_enable_msi(pci_device_t *dev, uint8_t vector, uint8_t lapic_id) {
    if (!dev || dev->msi_cap_offset == 0) return false;

    uint8_t cap = dev->msi_cap_offset;
    uint16_t msg_ctrl = pci_read16(dev, cap + 0x02);
    bool is_64bit = (msg_ctrl & (1 << 7)) != 0;

    // x86 MSI Address Layout: 0xFEE00000 | (Destination ID << 12)
    uint32_t msg_addr_low = 0xFEE00000 | ((uint32_t)lapic_id << 12);
    uint32_t msg_data = vector; // Delivery mode 000 (Fixed), Edge triggered

    pci_write32(dev, cap + 0x04, msg_addr_low);

    if (is_64bit) {
        pci_write32(dev, cap + 0x08, 0x00000000); // Address High
        pci_write16(dev, cap + 0x0C, msg_data);
    } else {
        pci_write16(dev, cap + 0x08, msg_data);
    }

    // Command/Bus Master aktivieren
    uint16_t cmd = pci_read16(dev, 0x04);
    pci_write16(dev, 0x04, cmd | (1 << 2)); // Bit 2: Bus Master Enable

    // Enable-Bit im Message Control Register setzen
    msg_ctrl |= (1 << 0);
    pci_write16(dev, cap + 0x02, msg_ctrl);

    serial_printf(COM1, "PCI: Enabled MSI on [%02x:%02x.%d] -> Vector %u, LAPIC %u\n", dev->bus, dev->device, dev->function, vector, lapic_id);

    return true;
}