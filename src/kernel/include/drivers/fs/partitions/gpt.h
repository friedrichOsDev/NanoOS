/**
 * @file gpt.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define GPT_SIGNATURE 0x5452415020494645

typedef struct {
    uint8_t  boot_indicator; 
    uint8_t  starting_chs[3];
    uint8_t  os_type;        
    uint8_t  ending_chs[3]; 
    uint32_t starting_lba;  
    uint32_t ending_lba;    
} __attribute__((packed)) gpt_pmbr_entry_t;

typedef struct {
    uint8_t bootstrap[440];
    uint8_t disk_signature[4];
    uint16_t unused;
    gpt_pmbr_entry_t partitions[4];
    uint16_t signature;
} __attribute__((packed)) gpt_t;

typedef struct {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} __attribute__((packed)) gpt_guid_t;

typedef struct {
    gpt_guid_t partition_type_guid;
    gpt_guid_t unique_partition_guid;
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    uint16_t partition_name[36];
} __attribute__((packed)) gpt_partition_entry_t;

typedef struct {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    gpt_guid_t disk_guid;
    uint64_t partition_entry_lba;
    uint32_t num_partition_entries;
    uint32_t partition_entry_size;
    uint32_t partition_entry_array_crc32;
    uint8_t  reserved2[420];
} __attribute__((packed)) gpt_header_t;

bool gpt_parse(uint8_t disk_index);