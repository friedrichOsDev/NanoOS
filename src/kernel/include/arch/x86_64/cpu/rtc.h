/**
 * @file rtc.h
 * @brief Schnittstelle für die Real-Time Clock (CMOS RTC).
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>

/** @brief I/O-Port zur Auswahl des CMOS-Registers */
#define CMOS_ADDR 0x70
/** @brief I/O-Port zum Lesen/Schreiben von CMOS-Daten */
#define CMOS_DATA 0x71

/** @brief Register-Index für Sekunden */
#define RTC_REG_SECONDS 0x00
/** @brief Register-Index für Minuten */
#define RTC_REG_MINUTES 0x02
/** @brief Register-Index für Stunden */
#define RTC_REG_HOURS 0x04
/** @brief Register-Index für den Tag des Monats */
#define RTC_REG_DAY 0x07
/** @brief Register-Index für den Monat */
#define RTC_REG_MONTH 0x08
/** @brief Register-Index für das Jahr */
#define RTC_REG_YEAR 0x09
/** @brief Register-Index für das Jahrhundert (nicht auf allen Systemen vorhanden) */
#define RTC_REG_CENTURY 0x32
/** @brief Statusregister A (Enthält Update-in-Progress Flag) */
#define RTC_REG_STATUS_A 0x0A
/** @brief Statusregister B (Enthält Format-Flags wie BCD/Binär und 12/24h) */
#define RTC_REG_STATUS_B 0x0B

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

/**
 * @brief Initialisiert die RTC und ermittelt die Startzeit des Systems.
 */
void rtc_init();

/**
 * @brief Gibt die beim Systemstart ausgelesene RTC-Zeit zurück.
 * @return `rtc_time_t` Struktur des Boot-Zeitpunkts.
 */
rtc_time_t rtc_get_boot_time();

/**
 * @brief Konvertiert eine RTC-Zeitstruktur in einen Unix-Timestamp (Sekunden seit 01.01.1970).
 * @param t Zeiger auf die umzuwandelnde `rtc_time_t` Struktur.
 * @return Unix-Timestamp in Sekunden.
 */
uint64_t rtc_to_unix(const rtc_time_t *t);

/**
 * @brief Berechnet die aktuelle Systemzeit als Unix-Timestamp unter Nutzung der HPET-Uptime.
 * @return Aktueller Unix-Timestamp in Sekunden.
 */
uint64_t time_get_unix();

/**
 * @brief Gibt die aktuelle Systemzeit als `rtc_time_t` Struktur zurück.
 * @return Aktuelle Systemzeit.
 */
rtc_time_t time_get_now();