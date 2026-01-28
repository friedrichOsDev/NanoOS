/*
 * @file pci.c
 * @brief PCI (Peripheral Component Interconnect) implementation
 * @author friedrichOsDev
 */

#include <pci.h>
#include <ata.h>
#include <ahci.h>
#include <io.h>
#include <heap.h>
#include <serial.h>

/*
 * Array of storage drivers
 */
static pci_driver_t storage_drivers[] = {
    {0x01, 0x01, ata_pci_init, "ATA (IDE) Controller"},
    //{0x01, 0x06, ahci_pci_init, "SATA (AHCI) Controller"},
    {0, 0, NULL, NULL} // Sentinel
};

/*
 * Read from PCI configuration space
 * @param bus PCI bus number
 * @param slot PCI slot number
 * @param func PCI function number
 * @param offset Offset within the PCI configuration space
 * @return 32-bit value read from the specified PCI configuration space
 */
uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    outl(0xCF8, address);
    return inl(0xCFC);
}

/*
 * Scan the PCI bus for devices and initialize drivers
 * @param void
 */
void pci_scan(void) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vendor = pci_config_read(bus, slot, 0, 0x00) & 0xFFFF;
            if (vendor == 0xFFFF) continue;

            uint8_t header_type = (pci_config_read(bus, slot, 0, 0x0C) >> 16) & 0xFF;
            uint8_t func_count = (header_type & 0x80) ? 8 : 1;

            for (uint8_t func = 0; func < func_count; func++) {
                uint32_t dev_vendor = pci_config_read(bus, slot, func, 0x00);
                uint16_t v_id = dev_vendor & 0xFFFF;
                if (v_id == 0xFFFF) continue;

                uint32_t rev_class = pci_config_read(bus, slot, func, 0x08);
                uint8_t class_code = (rev_class >> 24) & 0xFF;
                uint8_t subclass = (rev_class >> 16) & 0xFF;

                for (int i = 0; storage_drivers[i].init != NULL; i++) {
                    if (class_code == storage_drivers[i].class_code && subclass == storage_drivers[i].subclass) {
                        serial_puts("Found: ");
                        serial_puts(storage_drivers[i].name);
                        serial_puts("\n");

                        pci_device_t* pci_dev = kmalloc(sizeof(pci_device_t));
                        pci_dev->bus = bus;
                        pci_dev->slot = slot;
                        pci_dev->function = func;
                        pci_dev->vendor_id = v_id;
                        pci_dev->device_id = (dev_vendor >> 16) & 0xFFFF;
                        pci_dev->class_code = class_code;
                        pci_dev->subclass_code = subclass;

                        storage_drivers[i].init(pci_dev);
                    }
                }
            }
        }
    }
}

/*
 * A function to initialize PCI
 * @param void
 */
void pci_init(void) {
    serial_puts("PCI: Initializing scan...\n");
    pci_scan();
}