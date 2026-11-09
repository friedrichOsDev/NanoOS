/**
 * @file ringbuffer.c
 * @brief Ring buffer implementation
 * @author friedrichOsDev
 */

#include <lib/types/ringbuffer.h>
#include <lib/util/mem.h>
#include <mm/memory.h>
#include <pch.h>

/**
 * @brief Initialize a ring buffer.
 *
 * @param capacity The capacity of the ring buffer.
 * @param out Pointer to the ring buffer structure.
 * @return E_OK on success, E_INVAL on invalid input.
 */
kerr_t ringbuffer_create(size_t capacity, ringbuffer_t **out) {
    if (!out || capacity == 0) {
        return E_INVAL;
    }

    virt_addr_t rb_mem = 0;
    K_TRY(kzalloc(sizeof(ringbuffer_t), &rb_mem));
    ringbuffer_t *rb = (ringbuffer_t *)rb_mem;

    virt_addr_t buf_mem = 0;
    kerr_t err = kzalloc(capacity, &buf_mem);
    if (k_failed(err)) {
        kfree((virt_addr_t)rb);
        return err;
    }

    rb->buffer = (uint8_t *)buf_mem;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
    rb->lock = (spinlock_t)SPINLOCK_INIT;

    *out = rb;
    return E_OK;
}

/**
 * @brief Deinitialize a ring buffer.
 *
 * @param rb Ringbuffer to destroy
 * @return E_OK on success, E_INVAL on invalid input.
 */
kerr_t ringbuffer_destroy(ringbuffer_t *rb) {
    if (!rb) {
        return E_INVAL;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&rb->lock);
    if (rb->buffer) {
        kfree((virt_addr_t)rb->buffer);
        rb->buffer = NULL;
    }
    spinlock_release_irqrestore(&rb->lock, flags);

    kfree((virt_addr_t)rb);
    return E_OK;
}

/**
 * @brief Write data to a ring buffer.
 *
 * @param rb Ringbuffer to write to.
 * @param data Data to write.
 * @param size Number of bytes to write.
 * @return Number of bytes written, or 0 on error.
 */
size_t ringbuffer_write(ringbuffer_t *rb, const void *data, size_t size) {
    if (!rb || !data || size == 0) {
        return 0;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&rb->lock);

    size_t free_space = rb->capacity - rb->size;
    if (size > free_space) {
        size = free_space; // Nur so viel schreiben, wie Platz ist
    }

    if (size == 0) {
        spinlock_release_irqrestore(&rb->lock, flags);
        return 0;
    }

    const uint8_t *src = (const uint8_t *)data;
    size_t first_chunk = rb->capacity - rb->head;

    if (size <= first_chunk) {
        memcpy(rb->buffer + rb->head, src, size);
        rb->head = (rb->head + size) % rb->capacity;
    } else {
        memcpy(rb->buffer + rb->head, src, first_chunk);
        size_t second_chunk = size - first_chunk;
        memcpy(rb->buffer, src + first_chunk, second_chunk);
        rb->head = second_chunk;
    }

    rb->size += size;

    spinlock_release_irqrestore(&rb->lock, flags);
    return size;
}

/**
 * @brief Read data from the ring buffer.
 *
 * @param rb Ring buffer to read from.
 * @param data Buffer to store the read data.
 * @param size Number of bytes to read.
 * @return Number of bytes read or 0 if an error occurred.
 */
size_t ringbuffer_read(ringbuffer_t *rb, void *data, size_t size) {
    if (!rb || !data || size == 0) {
        return 0;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&rb->lock);

    if (size > rb->size) {
        size = rb->size; // Nur so viel lesen, wie vorhanden ist
    }

    if (size == 0) {
        spinlock_release_irqrestore(&rb->lock, flags);
        return 0;
    }

    uint8_t *dest = (uint8_t *)data;
    size_t first_chunk = rb->capacity - rb->tail;

    if (size <= first_chunk) {
        memcpy(dest, rb->buffer + rb->tail, size);
        rb->tail = (rb->tail + size) % rb->capacity;
    } else {
        memcpy(dest, rb->buffer + rb->tail, first_chunk);
        size_t second_chunk = size - first_chunk;
        memcpy(dest + first_chunk, rb->buffer, second_chunk);
        rb->tail = second_chunk;
    }

    rb->size -= size;

    spinlock_release_irqrestore(&rb->lock, flags);
    return size;
}

/**
 * @brief Check how many bytes are available in the ring buffer.
 *
 * @param rb Ring buffer to check.
 * @return Number of bytes available in the ring buffer.
 */
size_t ringbuffer_available(ringbuffer_t *rb) {
    if (!rb)
        return 0;
    irq_flags_t flags = spinlock_acquire_irqsave(&rb->lock);
    size_t s = rb->size;
    spinlock_release_irqrestore(&rb->lock, flags);
    return s;
}

/**
 * @brief Check if the ring buffer is empty.
 *
 * @param rb Ring buffer to check.
 * @return True if the ring buffer is empty, false otherwise.
 */
bool ringbuffer_is_empty(ringbuffer_t *rb) {
    return ringbuffer_available(rb) == 0;
}

/**
 * @brief Check if the ring buffer is full.
 *
 * @param rb Ring buffer to check.
 * @return True if the ring buffer is full, false otherwise.
 */
bool ringbuffer_is_full(ringbuffer_t *rb) {
    if (!rb)
        return false;
    irq_flags_t flags = spinlock_acquire_irqsave(&rb->lock);
    bool full = (rb->size >= rb->capacity);
    spinlock_release_irqrestore(&rb->lock, flags);
    return full;
}
