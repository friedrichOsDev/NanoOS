/**
 * @file ringbuffer.h
 * @brief Ring buffer implementation
 * @author friedrichOsDev
 */

#pragma once

#include <kernel/sync.h>
#include <pch.h>

/** @brief Ring buffer structure. */
typedef struct {
    uint8_t *buffer; /**< Buffer pointer */
    size_t capacity; /**< Capacity in bytes */
    size_t head;     /**< Write index */
    size_t tail;     /**< Read index */
    size_t size;     /**< Current number of bytes in the buffer */
    spinlock_t lock; /**< Lock for Thread safety */
} ringbuffer_t;

kerr_t ringbuffer_create(size_t capacity, ringbuffer_t **out);
kerr_t ringbuffer_destroy(ringbuffer_t *rb);
size_t ringbuffer_write(ringbuffer_t *rb, const void *data, size_t size);
size_t ringbuffer_read(ringbuffer_t *rb, void *data, size_t size);
size_t ringbuffer_available(ringbuffer_t *rb);
bool ringbuffer_is_empty(ringbuffer_t *rb);
bool ringbuffer_is_full(ringbuffer_t *rb);
