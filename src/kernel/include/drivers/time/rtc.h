/**
 * @file rtc.h
 * @author friedrichOsDev
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS 0x04
#define RTC_DAY 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09

#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B

/**
 * @brief Structure representing the current date and time from the RTC.
 */
typedef struct {
    uint8_t seconds; /**< Seconds (0-59) */
    uint8_t minutes; /**< Minutes (0-59) */
    uint8_t hours;   /**< Hours (0-23) */
    uint8_t day;     /**< Day of the month (1-31) */
    uint8_t month;   /**< Month (1-12) */
    uint16_t year;   /**< Full year (e.g., 2023) */
} rtc_time_t;

void rtc_init(void);
rtc_time_t rtc_get_time(void);
const char* rtc_get_time_format(uint32_t type);
