/*
 * @file timer.h
 * @brief Header file for timer driver implementation
 * @author friedrichOsDev
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

uint32_t timer_get_ticks(void);
void timer_init(uint32_t frequency);

#endif // TIMER_H