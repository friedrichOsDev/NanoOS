/**
 * @file hid_test.h
 * @brief Debug-only HID event monitor.
 */
#pragma once

#include <stddef.h>

/**
 * @brief Consume and log up to max_events from the shared input queue.
 *
 * Intended for a debug kernel with no other HID queue consumer.
 * Call from thread context, never from an interrupt handler.
 *
 * @return Number of events consumed.
 */
size_t hid_test_poll(size_t max_events);
