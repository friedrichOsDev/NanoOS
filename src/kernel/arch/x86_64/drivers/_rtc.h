#pragma once

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
