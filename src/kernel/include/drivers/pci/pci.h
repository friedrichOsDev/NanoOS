/**
 * @file pci.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND   0x04
#define PCI_STATUS    0x06
#define PCI_REVISION_ID 0x08
#define PCI_PROG_IF   0x09
#define PCI_SUBCLASS  0x0A
#define PCI_CLASS     0x0B
#define PCI_CACHE_LINE_SIZE 0x0C
#define PCI_LATENCY_TIMER   0x0D
#define PCI_HEADER_TYPE     0x0E
#define PCI_BIST            0x0F
#define PCI_BAR0            0x10
#define PCI_BAR1            0x14
#define PCI_BAR2            0x18
#define PCI_BAR3            0x1C
#define PCI_BAR4            0x20
#define PCI_BAR5            0x24
#define PCI_CAP_PTR         0x34
#define PCI_INTERRUPT_LINE  0x3C

#define PCI_HEADER_TYPE_DEVICE 0x00
#define PCI_HEADER_TYPE_BRIDGE 0x01
#define PCI_HEADER_TYPE_CARDBUS 0x02

#define PCI_BAR_COUNT 6
#define PCI_BAR_MEM 0x00
#define PCI_BAR_IO 0x01

/**
 * @brief Structure representing a PCI Base Address Register (BAR).
 */
typedef struct {
    uint32_t base_address;
    uint32_t size;
    uint8_t type;
} pci_bar_t;

/**
 * @brief Structure representing a PCI device and its basic identification information.
 */
typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
} pci_device_t;

/**
 * @brief Structure representing a PCI driver.
 */
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    const char * name;
    void (* init)(pci_device_t * dev);
} pci_driver_t;

uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t data);

void pci_init();
void pci_enumerate_devices();
pci_device_t pci_get_device(uint16_t vendor_id, uint16_t device_id);
pci_bar_t pci_get_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index);
void pci_print_device_info(pci_device_t * dev);