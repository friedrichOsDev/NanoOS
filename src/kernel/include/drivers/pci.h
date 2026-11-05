/**
 * @file pci.h
 * @brief PCI definitions and structures.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/** @brief Represents a discovered PCI device and its configuration state. */
typedef struct pci_device {
    uint16_t vendor_id;  /**< Vendor ID */
    uint16_t device_id;  /**< Device ID */
    uint8_t class_code;  /**< Device class code */
    uint8_t subclass;    /**< Subclass code */
    uint8_t prog_if;     /**< Programming interface */
    uint8_t revision_id; /**< Revision number */

    uint8_t bus;      /**< PCI bus number */
    uint8_t device;   /**< PCI device number */
    uint8_t function; /**< PCI function number */

    virt_addr_t config_space_virt; /**< Virtual base address of the ECAM configuration space */

    uint64_t bar[6];      /**< Base Address Registers (BAR 0-5) */
    uint64_t bar_size[6]; /**< Memory size of each BAR */
    bool is_mmio_bar[6];  /**< True if MMIO BAR, false if I/O port BAR */
    bool is_64bit_bar[6]; /**< True if 64-bit addressing is used */

    uint8_t msi_cap_offset;  /**< Offset of the MSI capability in config space (0 if not present) */
    uint8_t msix_cap_offset; /**< Offset of the MSI-X capability in config space (0 if not present) */

    struct pci_device *next; /**< Pointer to the next device in the linked list */
} pci_device_t;

typedef bool (*pci_driver_probe_t)(pci_device_t *dev);

/** @brief Represents a device driver registered within the PCI subsystem. */
typedef struct pci_driver {
    const char *name;         /**< Name of the driver */
    uint16_t vendor_id;       /**< Target vendor ID (0xFFFF for wildcard/any) */
    uint16_t device_id;       /**< Target device ID (0xFFFF for wildcard/any) */
    uint8_t class_code;       /**< Target device class code (0xFF for wildcard/any) */
    uint8_t subclass;         /**< Target device subclass code (0xFF for wildcard/any) */
    pci_driver_probe_t probe; /**< Callback function to initialize the device */
    struct pci_driver *next;  /**< Pointer to the next driver in the list */
} pci_driver_t;

kerr_t pci_init();
kerr_t pci_register_driver(pci_driver_t *driver);
