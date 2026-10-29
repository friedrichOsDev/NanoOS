/**
 * @file rtc.h
 * @brief Schnittstelle für die Real-Time Clock (CMOS RTC).
 * @author friedrichOsDev
 */

#pragma once

#include <pch.h>

/**
 * @brief Struktur zur Speicherung von Datums- und Zeitangaben der RTC.
 */
typedef struct {
    uint8_t second; /**< Sekunden (0–59) */
    uint8_t minute; /**< Minuten (0–59) */
    uint8_t hour;   /**< Stunden (0–23) */
    uint8_t day;    /**< Tag des Monats (1–31) */
    uint8_t month;  /**< Monat (1–12) */
    uint16_t year;  /**< Jahr (z. B. 2026) */
} rtc_time_t;

void rtc_init();
rtc_time_t rtc_get_boot_time();
uint64_t rtc_to_unix(const rtc_time_t *t);
uint64_t time_get_unix();
rtc_time_t time_get_now();
