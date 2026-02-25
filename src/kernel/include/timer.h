/**
 * @file timer.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TIMER_FREQUENCY 60 /**< PIT frequency in Hz. */
#define TIMER_MAX_EVENTS 64 /**< Maximum number of concurrent timer events. */

/**
 * @brief Structure representing a timed event.
 */
typedef struct {
    uint32_t event_id; /**< Unique ID for the event. */
    /** @brief Function to call when the event triggers. */
    void (*handler)(void);
    uint32_t interval;
    uint32_t target_tick;
    bool repeat;
    bool active;
} event_t;

void timer_init(void);
uint32_t timer_add_event(event_t event);
void timer_remove_event(uint32_t event_id);
uint32_t timer_get_ticks(void);
void sleep_ms(uint32_t ms);
