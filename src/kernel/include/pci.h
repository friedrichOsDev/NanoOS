/*
 * @file pci.h
 * @brief Header file for PCI (Peripheral Component Interconnect) implementation
 * @author friedrichOsDev
 */

#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <stddef.h>

#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND 0x04
#define PCI_CLASS 0x0B
#define PCI_SUBCLASS 0x0A
#define PCI_BAR0 0x10

/*
 * Structure representing a PCI device
 */
typedef struct {
    uint16_t bus;
    uint8_t slot;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass_code;
} pci_device_t;

/*
 * Structure representing a PCI driver
 */
typedef struct {
    uint8_t class_code;
    uint8_t subclass;
    void (*init)(pci_device_t* device);
    char* name;
} pci_driver_t;

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_init(void);

#endif //PCI_H