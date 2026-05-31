/**
 * @file mbr.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t drive_attributes;
    uint8_t chs_start[3];
    uint8_t partition_type;
    uint8_t chs_end[3];
    uint32_t lba_start;
    uint32_t sector_count;
} __attribute__((packed)) mbr_partition_entry_t;

typedef struct {
    uint8_t bootstrap[440];
    uint8_t disk_signature[4];
    uint16_t unused;
    mbr_partition_entry_t partitions[4];
    uint16_t signature;
} __attribute__((packed)) mbr_t;

bool mbr_parse(uint8_t disk_index);