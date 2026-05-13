/**
 * @file ata.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <storage.h>

#define ATA_DATA 0x1F0
#define ATA_ERROR 0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE_SELECT 0x1F6
#define ATA_COMMAND 0x1F7
#define ATA_STATUS 0x1F7

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DSC 0x10
#define ATA_SR_DRQ 0x08
#define ATA_SR_CORR 0x04
#define ATA_SR_IDX 0x02
#define ATA_SR_ERR 0x01

typedef struct {
    disk_t base;
    uint8_t drive_id;
} ata_disk_t;

void ata_init();
void ata_identify(uint8_t drive);
uint8_t ata_read_sectors(disk_t* self, uint64_t lba, uint32_t count, void* buffer);
uint8_t ata_write_sectors(disk_t* self, uint64_t lba, uint32_t count, const void* buffer);