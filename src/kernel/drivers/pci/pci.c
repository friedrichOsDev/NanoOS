/**
 * @file pci.c
 * @author friedrichOsDev
 */

#include <pci.h>
#include <io.h>
#include <serial.h>
#include <kernel.h>
#include <heap.h>
#include <ahci.h>
#include <rtx3050.h>

uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t ldev = (uint32_t)device;
    uint32_t lfunc = (uint32_t)function;

    address = (uint32_t)((lbus << 16) | (ldev << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t data) {
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t ldev = (uint32_t)device;
    uint32_t lfunc = (uint32_t)function;

    address = (uint32_t)((lbus << 16) | (ldev << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, data);
}

/**
 * @brief Database of supported PCI drivers and their initialization functions.
 */
size_t driver_database_size = 0;
pci_driver_t * driver_database = NULL;

pci_driver_t ahci_driver = {
    .vendor_id = 0,
    .class_code = 0x01, // Mass Storage
    .subclass = 0x06, // SATA Controller
    .prog_if = 0x01, // AHCI HBA
    .name = "AHCI SATA Controller",
    .init = ahci_init_device
};

pci_driver_t rtx3050 = {
    .vendor_id = 0x10DE,
    .device_id = 0x2584,
    .name = "NVIDIA GeForce RTX 3050",
    .init = rtx3050_init_device
};

void pci_init() {
    pci_register_driver(&ahci_driver);
    pci_register_driver(&rtx3050);
    init_state = INIT_PCI;
}

void pci_register_driver(pci_driver_t * driver) {
    if (driver_database_size == 0) {
        driver_database = (pci_driver_t *)kmalloc(sizeof(pci_driver_t) * 2);
        driver_database[0] = *driver;
        driver_database[1] = (pci_driver_t){0, 0, 0, 0, 0, NULL, NULL};
        driver_database_size = 2;
    } else {
        driver_database_size++;
        driver_database = (pci_driver_t *)krealloc((virt_addr_t)driver_database, sizeof(pci_driver_t) * driver_database_size);
        driver_database[driver_database_size - 2] = *driver;
        driver_database[driver_database_size - 1] = (pci_driver_t){0, 0, 0, 0, 0, NULL, NULL};
    }
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            pci_check_dev((uint8_t)bus, device, driver);
        }
    }
}

void pci_check_dev(uint8_t bus, uint8_t device, pci_driver_t * driver) {
    uint8_t function = 0;

    // read vendor
    uint32_t res = pci_config_read_dword(bus, device, function, PCI_VENDOR_ID);
    uint16_t vendor_id = (uint16_t)(res & 0xFFFF);
    
    if (vendor_id == 0xFFFF) return;

    uint32_t header_res = pci_config_read_dword(bus, device, function, PCI_HEADER_TYPE);
    uint8_t header_type = (uint8_t)((header_res >> 16) & 0xFF);

    uint8_t num_functions = (header_type & 0x80) ? 8 : 1;

    for (function = 0; function < num_functions; function++) {
        uint32_t dev_res = pci_config_read_dword(bus, device, function, PCI_VENDOR_ID);
        uint16_t func_vendor_id = (uint16_t)(dev_res & 0xFFFF);
        uint16_t device_id = (uint16_t)((dev_res >> 16) & 0xFFFF);

        if (func_vendor_id == 0xFFFF) continue;

        uint32_t class_res = pci_config_read_dword(bus, device, function, PCI_REVISION_ID);
        uint8_t class_code = (uint8_t)((class_res >> 24) & 0xFF);
        uint8_t subclass = (uint8_t)((class_res >> 16) & 0xFF);
        uint8_t prog_if = (uint8_t)((class_res >> 8) & 0xFF);
        
        uint8_t is_match = 0;

        if (driver->vendor_id != 0) {
            // Try match via exact vendor/device ID
            if (driver->vendor_id == func_vendor_id && driver->device_id == device_id) {
                is_match = 1;
            }
        } else {
            // Try match via class/subclass/prog_if
            if (driver->class_code == class_code && 
                driver->subclass == subclass && 
                driver->prog_if == prog_if) {
                is_match = 1;
            }
        }

        if (!is_match) continue;

        pci_device_t dev = {
            .bus = bus,
            .device = device,
            .function = function,
            .vendor_id = func_vendor_id,
            .device_id = device_id,
            .class_code = class_code,
            .subclass = subclass,
            .prog_if = prog_if
        };

        if (driver->init) {
            driver->init(&dev);
        }

        pci_print_device_info(&dev);

        for (uint8_t i = 0; i < PCI_BAR_COUNT; i++) {
            pci_bar_t bar = pci_get_bar(bus, device, function, i);
            if (bar.base_address != 0) {
                serial_printf("  BAR%d: %s Base: %08x Size: %08x\n", i, (bar.type == PCI_BAR_IO ? "I/O" : "Mem"), bar.base_address, bar.size);
            }
        }
        uint32_t intr = pci_config_read_dword(bus, device, function, PCI_INTERRUPT_LINE);
        serial_printf("  Interrupt Line: %d\n", (uint8_t)(intr & 0xFF));
    }
}

pci_device_t pci_get_device(uint16_t vendor_id, uint16_t device_id) {
    pci_device_t dev = {0};
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            uint32_t res = pci_config_read_dword((uint8_t)bus, device, 0, PCI_VENDOR_ID);
            if ((uint16_t)(res & 0xFFFF) == 0xFFFF) continue;

            uint32_t header_res = pci_config_read_dword((uint8_t)bus, device, 0, PCI_HEADER_TYPE);
            uint8_t header_type = (uint8_t)((header_res >> 16) & 0xFF);
            uint8_t num_functions = (header_type & 0x80) ? 8 : 1;

            for (uint8_t function = 0; function < num_functions; function++) {
                uint32_t dev_res = pci_config_read_dword((uint8_t)bus, device, function, PCI_VENDOR_ID);
                if ((uint16_t)(dev_res & 0xFFFF) == vendor_id && (uint16_t)((dev_res >> 16) & 0xFFFF) == device_id) {
                    uint32_t class_res = pci_config_read_dword((uint8_t)bus, device, function, PCI_REVISION_ID);
                    
                    dev.bus = (uint8_t)bus;
                    dev.device = device;
                    dev.function = function;
                    dev.vendor_id = vendor_id;
                    dev.device_id = device_id;
                    
                    dev.class_code = (uint8_t)((class_res >> 24) & 0xFF);
                    dev.subclass = (uint8_t)((class_res >> 16) & 0xFF);
                    dev.prog_if = (uint8_t)((class_res >> 8) & 0xFF);
                    
                    return dev;
                }
            }
        }
    }
    return dev;
}

pci_bar_t pci_get_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index) {
    pci_bar_t bar = {0};
    if (bar_index >= PCI_BAR_COUNT) return bar;

    uint8_t offset = PCI_BAR0 + (bar_index * 4);
    uint32_t bar_value = pci_config_read_dword(bus, device, function, offset);

    bar.type = (bar_value & 0x01) ? PCI_BAR_IO : PCI_BAR_MEM;

    uint32_t mask = (bar.type == PCI_BAR_IO) ? 0xFFFFFFFC : 0xFFFFFFF0;
    bar.base_address = bar_value & mask;

    // Calculate size
    pci_config_write_dword(bus, device, function, offset, 0xFFFFFFFF);
    uint32_t size_readback = pci_config_read_dword(bus, device, function, offset);
    pci_config_write_dword(bus, device, function, offset, bar_value);

    mask = (bar.type == PCI_BAR_IO) ? 0xFFFFFFFC : 0xFFFFFFF0;
    uint32_t size = size_readback & mask;

    if (size == 0) bar.size = 0;
    else bar.size = ~size + 1;

    return bar;
}

void pci_print_device_info(pci_device_t * dev) {
    serial_printf("PCI Device: %02x:%02x.%d ID: %04x:%04x Class: %02x Sub: %02x\n", dev->bus, dev->device, dev->function, dev->vendor_id, dev->device_id, dev->class_code, dev->subclass);
}
