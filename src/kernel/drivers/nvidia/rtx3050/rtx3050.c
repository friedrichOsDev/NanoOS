/**
 * @file rtx3050.c
 * @author friedrichOsDev
 */

#include <rtx3050.h>
#include <serial.h>

/**
 * @brief Initializes the NVIDIA RTX 3050 GPU.
 * @param dev Pointer to the PCI device structure.
 */
void rtx3050_init(pci_device_t * dev) {
    serial_printf("RTX 3050 initialized at PCI %02x:%02x.%x\n", dev->bus, dev->device, dev->function);
}
