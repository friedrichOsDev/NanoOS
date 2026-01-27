/*
 * @file ata.h
 * @brief Header file for ATA (Advanced Technology Attachment) disk driver
 * @author friedrichOsDev
 */

#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CTRL 0x376

/*
 * ATA bus type enumeration
 */
typedef enum {
    ATA_BUS_PRIMARY,
    ATA_BUS_SECONDARY
} ATA_BUS;

/*
 * ATA drive type enumeration
 */
typedef enum {
    ATA_DRIVE_MASTER = 0x00,
    ATA_DRIVE_SLAVE = 0x01
} ATA_DRIVE_TYPE;

/*
 * Structure to represent an ATA bus
 */
typedef struct {
    uint16_t io_base;
    uint16_t ctrl_base;
    uint8_t irq;
} ata_bus_info_t;

void ata_init(void);
uint8_t ata_read_sector(ATA_BUS bus, ATA_DRIVE_TYPE drive, uint32_t lba, uint8_t* buffer);
uint8_t ata_write_sector(ATA_BUS bus, ATA_DRIVE_TYPE drive, uint32_t lba, uint8_t* buffer);

#endif // ATA_H