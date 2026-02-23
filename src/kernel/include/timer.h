/*
 * @file timer.h
 * @brief Header file for timer driver
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TIMER_FREQUENCY 60
#define TIMER_MAX_EVENTS 64

// Timer event structure
typedef struct {
    uint32_t event_id;
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
