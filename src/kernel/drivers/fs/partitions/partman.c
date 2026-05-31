/**
 * @file partman.c
 * @author friedrichOsDev
 */

#include <partman.h>
#include <string.h>
#include <storage.h>
#include <serial.h>
#include <console.h>
#include <print.h>
#include <mbr.h>

static partition_t global_partitions[MAX_PARTITIONS];
static uint32_t partition_count = 0;

void partman_init() {
    memset(global_partitions, 0, sizeof(global_partitions));
    partition_count = 0;

    for (uint8_t i = 0; i < storage_get_disk_count(); i++) {
        mbr_parse(i);
    }   
}

int32_t partman_register_partition(uint8_t disk_index, uint32_t start_lba, uint32_t sector_count, uint8_t type, const char* name) {
    if (partition_count >= MAX_PARTITIONS) {
        serial_printf("Partman: Error: Maximum partition count reached\n");
        return -1;
    }

    uint32_t id = partition_count++;
    global_partitions[id].disk_index = disk_index;
    global_partitions[id].start_lba = start_lba;
    global_partitions[id].sector_count = sector_count;
    global_partitions[id].partition_type = type;
    global_partitions[id].used = true;

    if (name) {
        strncpy(global_partitions[id].name, name, sizeof(global_partitions[id].name) - 1);
    } else {
        snprintf(global_partitions[id].name, sizeof(global_partitions[id].name), "disk%d-part%d", disk_index, id);
    }

    serial_printf("Partman: Registered partition %d: %s (Disk %d, Start %u, Size %u)\n", id, global_partitions[id].name, disk_index, start_lba, sector_count);

    return (int32_t)id;
}

bool partman_read_partition(uint32_t partition_id, uint32_t relative_lba, uint32_t sector_count, uint8_t* buffer) {
    if (partition_id >= MAX_PARTITIONS || !global_partitions[partition_id].used) {
        serial_printf("Partman: Error: Invalid partition ID %u\n", partition_id);
        return false;
    }

    partition_t* part = &global_partitions[partition_id];
    
    if (relative_lba + sector_count > part->sector_count) {
        serial_printf("Partman: Error: Read exceeds partition bounds (Partition %s, Relative LBA %u, Sector Count %u)\n", part->name, relative_lba, sector_count);
        return false;
    }

    disk_t* disk = storage_get_disk(part->disk_index);
    if (!disk) {
        serial_printf("Partman: Error: No disk found for partition %s\n", part->name);
        return false;
    }

    uint32_t absolute_lba = part->start_lba + relative_lba;
    return storage_read(disk, absolute_lba, sector_count, buffer) == 0;
}

void partman_dump_info() {
    char buf[64];
    console_puts(U"Partition Table:\n");
    snprintf(buf, sizeof(buf), "Total Partitions: %u\n\n", partition_count);
    for (int i = 0; buf[i]; i++) console_putc((uint32_t)buf[i]);

    for (uint32_t i = 0; i < partition_count; i++) {
        partition_t* part = &global_partitions[i];
        if (!part->used) continue;

        snprintf(buf, sizeof(buf), "[Partition %u] %s\n", i, part->name);
        for (int j = 0; buf[j]; j++) console_putc((uint32_t)buf[j]);
        snprintf(buf, sizeof(buf), "  Disk: %u, Start LBA: %u, Sectors: %u, Type: %02X\n\n",
                 part->disk_index, part->start_lba, part->sector_count, part->partition_type);
        for (int j = 0; buf[j]; j++) console_putc((uint32_t)buf[j]);
    }
}