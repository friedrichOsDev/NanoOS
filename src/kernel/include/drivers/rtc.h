/**
 * @file rtc.h
 * @brief Real-Time Clock (RTC) definitions.
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/** @brief Structure representing RTC date and time values. */
typedef struct {
    uint8_t second; /**< Seconds (0–59) */
    uint8_t minute; /**< Minutes (0–59) */
    uint8_t hour;   /**< Hours (0–23) */
    uint8_t day;    /**< Day of the month (1–31) */
    uint8_t month;  /**< Month (1–12) */
    uint16_t year;  /**< Year (e.g., 2026) */
} rtc_time_t;

void rtc_init();
rtc_time_t rtc_get_boot_time();
uint64_t rtc_to_unix(const rtc_time_t *t);
uint64_t time_get_unix();
rtc_time_t time_get_now();
