/**
 * @file storage.h
 * @brief The HAL for all storage drivers
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#define MAX_DISKS 64

typedef enum {
    TYPE_UNKNOWN,
    TYPE_ATA,
    TYPE_AHCI
} disk_type_t;

typedef struct disk {
    char name[32];
    uint64_t total_sectors;
    uint32_t sector_size;
    disk_type_t type;

    uint8_t (*read)(struct disk* self, uint64_t lba, uint32_t count, void* buffer);
    uint8_t (*write)(struct disk* self, uint64_t lba, uint32_t count, const void* buffer);
} disk_t;

void storage_init();
disk_t* storage_get_disk(uint8_t index);
void storage_register_disk(disk_t* disk);
uint8_t storage_get_disk_count();
uint8_t storage_read(disk_t* disk, uint64_t lba, uint32_t count, void* buffer);
uint8_t storage_write(disk_t* disk, uint64_t lba, uint32_t count, const void* buffer);
void storage_dump_disk(disk_t* disk);
void storage_dump_info();