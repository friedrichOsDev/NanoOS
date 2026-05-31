/**
 * @file mbr.c
 * @author friedrichOsDev
 */

#include <mbr.h>
#include <storage.h>
#include <serial.h>
#include <partman.h>
#include <gpt.h>

bool mbr_parse(uint8_t disk_index) {
    mbr_t mbr;

    if (disk_index >= storage_get_disk_count()) {
        serial_printf("MBR: Error: Disk index %d out of bounds\n", disk_index);
        return false;
    }

    disk_t* disk = storage_get_disk(disk_index);
    if (!disk) {
        serial_printf("MBR: Error: No disk found at index %d\n", disk_index);
        return false;
    }

    if (storage_read(disk, 0, 1, &mbr) != 0) {
        serial_printf("MBR: Error reading MBR from disk %s\n", disk->name);
        return false;
    }

    if (mbr.signature != 0xAA55) {
        serial_printf("MBR: Invalid MBR signature on disk %s: %X\n", disk->name, mbr.signature);
        return false;
    }

    serial_printf("MBR: Found MBR on disk %s with signature %X\n", disk->name, mbr.signature);

    for (int i = 0; i < 4; i++) {
        mbr_partition_entry_t* part = &mbr.partitions[i];
        if (part->sector_count == 0) continue;

        if (part->partition_type == 0) {
            serial_printf("MBR: Skipping empty partition entry %d on disk %s\n", i, disk->name);
            continue;
        } else if (part->partition_type == 0xEE) {
            serial_printf("MBR: Found GPT protective partition on disk %s, skipping MBR parsing\n", disk->name);
            return gpt_parse(disk_index);
        }

        serial_printf("MBR: Partition %d: Type %02X, Start LBA %u, Sector Count %u\n", i, part->partition_type, part->lba_start, part->sector_count);

        partman_register_partition(disk_index, part->lba_start, part->sector_count, part->partition_type, NULL);
    }

    return true;
}