/**
 * @file pci.h
 * @brief PCIe / PCI MMCONFIG Subsystem & Driver Framework.
 * @author friedrichOsDev
 */

#pragma once

#include <arch/x86_64/cpu/acpi.h>
#include <arch/x86_64/mm/memdef.h>
#include <stdbool.h>
#include <stdint.h>

#define PCI_CLASS_UNCLASSIFIED 0x00
#define PCI_CLASS_STORAGE 0x01
#define PCI_CLASS_NETWORK 0x02
#define PCI_CLASS_DISPLAY 0x03
#define PCI_CLASS_MULTIMEDIA 0x04
#define PCI_CLASS_MEMORY 0x05
#define PCI_CLASS_BRIDGE 0x06
#define PCI_CLASS_SIMPLE_COMMUNICATION 0x07
#define PCI_CLASS_BASE_SYSTEM_PERIPHERAL 0x08
#define PCI_CLASS_INPUT_DEVICE 0x09
#define PCI_CLASS_DOCKING_STATION 0x0A
#define PCI_CLASS_PROCESSOR 0x0B
#define PCI_CLASS_SERIAL_BUS 0x0C
#define PCI_CLASS_WIRELESS 0x0D
#define PCI_CLASS_INTELLIGENT 0x0E
#define PCI_CLASS_SATELLITE_COMMUNICATION 0x0F
#define PCI_CLASS_ENCRYPTION 0x10
#define PCI_CLASS_SIGNAL_PROCESSING 0x11
#define PCI_CLASS_PROCESSING_ACCELERATOR 0x12
#define PCI_CLASS_NON_ESSENTIAL_INSTRUMENTATION 0x13
#define PCI_CLASS_CO_PROCESSOR 0x40
#define PCI_CLASS_UNASSIGNED 0xFF

#define PCI_CAP_ID_MSI   0x05
#define PCI_CAP_ID_MSIX  0x11

typedef struct pci_device {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision_id;

    uint8_t bus;
    uint8_t device;
    uint8_t function;

    virt_addr_t config_space_virt;

    uint64_t bar[6];
    uint64_t bar_size[6];
    bool is_mmio_bar[6];
    bool is_64bit_bar[6];

    uint8_t msi_cap_offset;
    uint8_t msix_cap_offset;

    struct pci_device *next;
} pci_device_t;

typedef bool (*pci_driver_probe_t)(pci_device_t *dev);

typedef struct pci_driver {
    const char *name;
    uint16_t vendor_id; // 0xFFFF = Wildcard
    uint16_t device_id; // 0xFFFF = Wildcard
    uint8_t class_code; // 0xFF = Wildcard
    uint8_t subclass;   // 0xFF = Wildcard
    pci_driver_probe_t probe;
    struct pci_driver *next;
} pci_driver_t;

void pci_init();
pci_device_t *pci_get_devices();
void pci_register_driver(pci_driver_t *driver);

uint32_t pci_read32(pci_device_t *dev, uint16_t offset);
uint16_t pci_read16(pci_device_t *dev, uint16_t offset);
uint8_t pci_read8(pci_device_t *dev, uint16_t offset);

void pci_write32(pci_device_t *dev, uint16_t offset, uint32_t val);
void pci_write16(pci_device_t *dev, uint16_t offset, uint16_t val);
void pci_write8(pci_device_t *dev, uint16_t offset, uint8_t val);

bool pci_enable_msi(pci_device_t *dev, uint8_t vector, uint8_t lapic_id);