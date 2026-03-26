/**
 * @file rtx3050.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pci.h>
#include <memory.h>

/**
 * @brief Structure representing the state and resources of an NVIDIA RTX 3050 GPU.
 */
typedef struct {
    pci_device_t pci_info;

    phys_addr_t mmio_base;
    phys_addr_t fb_base;
    size_t fb_size;

    void * command_ring_buffer;
    uint32_t ring_buffer_head;
    uint32_t ring_buffer_tail;
    
    bool hardware_acceleration_available;
    uint32_t max_width;
    uint32_t max_height;
} gpu_device_t;

/**
 * @brief Enumeration of supported GPU hardware operations.
 */
typedef enum {
    GPU_OP_COPY_RECT,     // BitBLT: VRAM to VRAM copy
    GPU_OP_UPLOAD_DATA,   // DMA: System RAM to VRAM
    GPU_OP_FILL_RECT,     // Hardware fill
    GPU_OP_BLIT,          // Alpha/Transparency blit
    GPU_OP_SET_STATE,     // Set colors, viewport, clipping
    GPU_OP_FLUSH,         // Submit command buffer
    GPU_OP_SYNC,          // Wait for GPU idle (fence)
    GPU_OP_SWAP_BUFFERS,  // V-Sync buffer swap
    GPU_OP_SET_OFFSET     // Hardware scroll (display start)
} gpu_op_t;

/**
 * @brief Structure representing a command to be executed by the GPU.
 */
typedef struct {
    gpu_op_t op;
    union {
        struct {
            uint32_t src_x, src_y;
            uint32_t dst_x, dst_y;
            uint32_t width, height;
        } copy_rect;
        struct {
            void * src_addr;
            uint32_t dst_offset;
            size_t size;
        } upload_data;
        struct {
            uint32_t x, y;
            uint32_t width, height;
            uint32_t color;
        } fill_rect;
        struct {
            uint32_t x, y;
            uint32_t width, height;
            uint32_t color;
            uint8_t alpha;
        } blit;
        struct {
            uint32_t viewport_width;
            uint32_t viewport_height;
            uint32_t clip_x, clip_y;
            uint32_t clip_w, clip_h;
        } state;
        struct {
            uint32_t offset;
        } display_offset;
    } data;
} gpu_command_t;

void rtx3050_init(pci_device_t * dev);
