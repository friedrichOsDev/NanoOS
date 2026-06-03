/**
 * @file partman.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MAX_PARTITIONS 16

typedef struct {
    uint8_t disk_index;
    uint32_t start_lba;
    uint32_t sector_count;
    uint8_t partition_type;
    char name[36];
    bool used;
} partition_t;

void partman_init();
partition_t* partman_get_partition(uint32_t partition_id);
uint32_t partman_get_partition_count();
int32_t partman_register_partition(uint8_t disk_index, uint32_t start_lba, uint32_t sector_count, uint8_t type, const char* name);
bool partman_read_partition(uint32_t partition_id, uint32_t relative_lba, uint32_t sector_count, uint8_t* buffer);
void partman_dump_info();